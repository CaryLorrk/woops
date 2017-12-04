#include "comm.h"

#include "util/logging.h"
#include "util/protobuf/ps_service.grpc.pb.h"

#include "server/server.h"
#include "client/client.h"
#include "server/ps_service_server.h"

namespace woops
{
Comm::Comm():
    barrier_cnt_(0) {}

void Comm::CreateTable(const TableConfig& config, size_t size) {
    server_->CreateTable(config, size);
}

void Comm::Update(int server, const std::string& tablename, const void* data,
        size_t size, int iteration) {
    if (server == this_host_) {
        server_->LocalUpdate(tablename, data, iteration);
    } else {
        rpc::UpdateRequest req;
        req.set_name(tablename);
        req.set_iteration(iteration);
        req.set_delta(data, size);
        std::lock_guard<std::mutex> lock(push_streams_mu_[server]);
        push_streams_[server]->Write(req);
    }
}

void Comm::Sync(int server, const std::string& tablename, int iteration) {
    rpc::PullRequest req;
    req.set_name(tablename);
    req.set_iteration(iteration);
    std::lock_guard<std::mutex> lock(pull_streams_mu_[server]);
    pull_streams_[server]->Write(req);
}

void Comm::Assign(int host, const std::string tablename, const void* data, size_t size) {
    rpc::AssignRequest req;
    req.set_tablename(tablename);
    req.set_parameter(data, size);

    grpc::ClientContext ctx;
    rpc::AssignResponse res;
    stubs_[host]->Assign(&ctx, req, &res);
}


void Comm::server_thread_func_() {
    rpc_server_->Wait();
}

void Comm::client_thread_func_(int server) {
    rpc::PullResponse res;
    while (pull_streams_[server]->Read(&res)) {
        client_->ServerAssign(server, res.name(), res.param().data(), res.iteration());
    }
}

using namespace std::chrono_literals;
void Comm::Initialize(const WoopsConfig& config, Client *client, Server *server) {
    this_host_ = config.this_host;
    staleness_ = config.staleness;
    port_ = config.port;
    hosts_ = config.hosts;

    client_ = client;
    server_ = server;

    /* server */
    grpc::ServerBuilder builder;
    builder.SetMaxMessageSize(100*1024*1024);
    builder.AddListeningPort("0.0.0.0:"+port_, grpc::InsecureServerCredentials());
    service_ = std::make_unique<PsServiceServer>(this, client_, server_);
    builder.RegisterService(service_.get());
    rpc_server_ = builder.BuildAndStart();
    server_thread_ = std::thread(&Comm::server_thread_func_, this);
    std::this_thread::sleep_for(100ms);

    /* stubs */
    LOG(INFO) << "Check servers:";
    grpc::ChannelArguments channel_args;
    channel_args.SetInt("grpc.max_message_length", 100*1024*1024);
    for (auto& host: hosts_) {
        while(1) {
            auto stub = rpc::PsService::NewStub(grpc::CreateCustomChannel(
                            host+":"+port_,
                            grpc::InsecureChannelCredentials(),
                            channel_args));
            grpc::ClientContext ctx;
            rpc::CheckAliveRequest req;
            rpc::CheckAliveResponse res;
            stub->CheckAlive(&ctx, req, &res);
            if (res.status()) {
                LOG(INFO) << host << " is up.";
                stubs_.push_back(std::move(stub));
                break;
            } else {
                LOG(WARNING) << "Failed to connect to " << host <<".";
                std::this_thread::sleep_for(1s);
            }
        }
    }

    /* Client */
    push_streams_mu_ = std::make_unique<std::mutex[]>(hosts_.size());
    pull_streams_mu_ = std::make_unique<std::mutex[]>(hosts_.size());
    for (size_t server = 0; server < hosts_.size(); server++) {
        auto push_ctx = std::make_unique<grpc::ClientContext>();
        push_ctx->AddMetadata("client", std::to_string(this_host_));
        push_streams_.push_back(stubs_[server]->Update(push_ctx.get()));
        push_ctxs_.push_back(std::move(push_ctx));

        auto pull_ctx = std::make_unique<grpc::ClientContext>();
        pull_ctx->AddMetadata("client", std::to_string(this_host_));
        pull_streams_.push_back(stubs_[server]->Pull(pull_ctx.get()));
        pull_ctxs_.push_back(std::move(pull_ctx));

        client_threads_.emplace_back(&Comm::client_thread_func_, this, server);
    }
    std::this_thread::sleep_for(100ms);
}

// Barrier
void Comm::Barrier() {
    grpc::ClientContext ctx;
    rpc::BarrierNotifyRequest req;
    rpc::BarrierNotifyResponse res;
    std::unique_lock<std::mutex> lock(barrier_mu_);
    if (this_host_ == 0) {
        barrier_cv_.wait(lock, [this]{return barrier_cnt_ >= hosts_.size() - 1;});
        barrier_cnt_ = 0;

        for (size_t host = 1; host < hosts_.size(); ++host) {
            stubs_[host]->BarrierNotify(&ctx, req, &res);
        }
        return;
    }    
    stubs_[0]->BarrierNotify(&ctx, req, &res);
    barrier_cv_.wait(lock);
}

void Comm::BarrierNotified() {
    std::unique_lock<std::mutex> lock(barrier_mu_);
    barrier_cnt_ += 1;
    lock.unlock();
    barrier_cv_.notify_all();
}

Comm::~Comm() {
    LOG(INFO) << "Clean up.";
    for (auto& s: pull_streams_) {
        s->WritesDone();
    }
    for (auto& s: push_streams_) {
        s->WritesDone();
    }
    rpc_server_->Shutdown();
    for (auto& s: pull_streams_) {
        auto status = s->Finish();
        if (!status.ok()) {
            LOG(FATAL) << "Failed to finish stream. Error code: " << status.error_code();
        }
    }
    for (auto& s: push_streams_) {
        auto status = s->Finish();
        if (!status.ok()) {
            LOG(FATAL) << "Failed to finish stream. Error code: " << status.error_code();
        }
    }
    server_thread_.join();
    for (auto& t: client_threads_) {
        t.join();
    }
}


} /* woops */ 
