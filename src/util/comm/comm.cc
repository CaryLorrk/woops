#include "comm.h"
#include "lib.h"

#include "util/logging.h"
#include "util/protobuf/ps_service.grpc.pb.h"
#include "util/placement/placement.h"

#include "server/server.h"
#include "client/client.h"

namespace woops
{
Comm::Comm():
    barrier_cnt_(0) {}

void Comm::CreateTable(const TableConfig& config, size_t size) {
    Lib::Server()->CreateTable(config, size);
}

void Comm::Update(int server, int id, std::string& data, int iteration) {
    if (server == Lib::ThisHost()) {
        Lib::Server()->Update(Lib::ThisHost(), id, data, iteration);
    } else {
        rpc::UpdateRequest req;
        req.set_tableid(id);
        req.set_iteration(iteration);
        req.set_delta(std::move(data));
        std::lock_guard<std::mutex> lock(update_streams_mu_[server]);
        update_streams_[server]->Write(req);
    }
}

void Comm::Pull(int server, int id, int iteration) {
    rpc::PullRequest req;
    req.set_tableid(id);
    req.set_iteration(iteration);
    std::lock_guard<std::mutex> lock(pull_streams_mu_[server]);
    pull_streams_[server]->Write(req);
}

void Comm::Push(int client, int id, const void* data, size_t size, int iteration) {
    rpc::PushRequest req;
    req.set_tableid(id);
    req.set_parameter(data, size);
    req.set_iteration(iteration);
    std::lock_guard<std::mutex> lock(push_streams_mu_[client]);
    push_streams_[client]->Write(req);
}

void Comm::ForceSync(Hostid host, Tableid id, std::string& data) {
    rpc::ForceSyncRequest req;
    req.set_tableid(id);
    req.set_parameter(std::move(data));

    grpc::ClientContext ctx;
    rpc::ForceSyncResponse res;
    stubs_[host]->ForceSync(&ctx, req, &res);
}


void Comm::server_thread_func_() {
    rpc_server_->Wait();
}

using namespace std::chrono_literals;
void Comm::Initialize() {
    /* server */
    grpc::ServerBuilder builder;
    builder.SetMaxMessageSize(100*1024*1024);
    builder.AddListeningPort("0.0.0.0:"+Lib::Port(), grpc::InsecureServerCredentials());
    service_ = std::make_unique<PsServiceServer>();
    builder.RegisterService(service_.get());
    rpc_server_ = builder.BuildAndStart();
    server_thread_ = std::thread(&Comm::server_thread_func_, this);
    std::this_thread::sleep_for(100ms);

    /* stubs */
    LOG(INFO) << "Check servers:";
    grpc::ChannelArguments channel_args;
    channel_args.SetInt("grpc.max_message_length", 100*1024*1024);
    for (auto& host: Lib::Hosts()) {
        while(1) {
            auto stub = rpc::PsService::NewStub(grpc::CreateCustomChannel(
                            host+":"+Lib::Port(),
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
    update_streams_mu_ = std::make_unique<std::mutex[]>(Lib::NumHosts());
    pull_streams_mu_ = std::make_unique<std::mutex[]>(Lib::NumHosts());
    push_streams_mu_ = std::make_unique<std::mutex[]>(Lib::NumHosts());
    for (size_t server = 0; server < Lib::NumHosts(); server++) {
        auto update_ctx = std::make_unique<grpc::ClientContext>();
        update_ctx->AddMetadata("from_host", std::to_string(Lib::ThisHost()));
        update_streams_.push_back(stubs_[server]->Update(update_ctx.get()));
        update_ctxs_.push_back(std::move(update_ctx));
        
        auto pull_ctx = std::make_unique<grpc::ClientContext>();
        pull_ctx->AddMetadata("from_host", std::to_string(Lib::ThisHost()));
        pull_streams_.push_back(stubs_[server]->Pull(pull_ctx.get()));
        pull_ctxs_.push_back(std::move(pull_ctx));

        auto push_ctx = std::make_unique<grpc::ClientContext>();
        push_ctx->AddMetadata("from_host", std::to_string(Lib::ThisHost()));
        push_streams_.push_back(stubs_[server]->Push(push_ctx.get()));
        push_ctxs_.push_back(std::move(push_ctx));
    }

    std::this_thread::sleep_for(100ms);
}

// Barrier
void Comm::Barrier() {
    std::unique_lock<std::mutex> lock(barrier_mu_);
    if (Lib::ThisHost() == 0) {
        barrier_cv_.wait(lock, [this]{return barrier_cnt_ >= Lib::NumHosts() - 1;});
        barrier_cnt_ = 0;

        for (size_t host = 1; host < Lib::NumHosts(); ++host) {
            grpc::ClientContext ctx;
            rpc::BarrierNotifyRequest req;
            rpc::BarrierNotifyResponse res;
            stubs_[host]->BarrierNotify(&ctx, req, &res);
        }
        return;
    }
    grpc::ClientContext ctx;
    rpc::BarrierNotifyRequest req;
    rpc::BarrierNotifyResponse res;
    stubs_[0]->BarrierNotify(&ctx, req, &res);
    barrier_cv_.wait(lock, [this]{return barrier_cnt_;});
    barrier_cnt_ = 0;
}

void Comm::barrier_notified_() {
    std::lock_guard<std::mutex> lock(barrier_mu_);
    barrier_cnt_ += 1;
    barrier_cv_.notify_all();
}

void Comm::SyncPlacement() {
    grpc::ClientContext ctx;
    rpc::SyncPlacementRequest req;
    rpc::SyncPlacementResponse res;
    stubs_[0]->SyncPlacement(&ctx, req, &res);
    Lib::Placement()->Deserialize(res.data());
}

void Comm::finish_() {
    if (Lib::ThisHost() == 0) {
        for (size_t host = Lib::NumHosts() - 1; host >= 0; --host) {
            grpc::ClientContext ctx;
            rpc::FinishRequest req;
            rpc::FinishResponse res;
            stubs_[host]->Finish(&ctx, req, &res);
        }
    }
    server_thread_.join();
}

Comm::~Comm() {
    LOG(INFO) << "Clean up.";
    finish_();
}


} /* woops */ 
