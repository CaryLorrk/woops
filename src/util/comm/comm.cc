#include "comm.h"
#include "lib.h"

#include "util/logging.h"
#include "util/protobuf/comm.grpc.pb.h"
#include "util/placement/placement.h"

#include "server/server.h"
#include "client/client.h"

namespace woops
{

using namespace std::chrono_literals;
void Comm::Initialize() {
    build_rpc_server();
    create_stubs();
    create_streams();
}

void Comm::build_rpc_server() {
    grpc::ServerBuilder builder;
    builder.SetMaxMessageSize(MAX_MESSAGE_SIZE);
    builder.AddListeningPort("0.0.0.0:"+Lib::Port(), grpc::InsecureServerCredentials());
    service_ = std::make_unique<CommServer>();
    builder.RegisterService(service_.get());
    rpc_server_ = builder.BuildAndStart();
    rpc_server_thread_ = std::thread(&Comm::rpc_server_func, this);
    std::this_thread::sleep_for(10ms);
}

void Comm::create_stubs() {
    LOG(INFO) << "Check servers:";
    grpc::ChannelArguments channel_args;
    channel_args.SetInt("grpc.max_message_length", MAX_MESSAGE_SIZE);
    for (auto&& host: Lib::Hosts()) {
        while(1) {
            auto stub = rpc::Comm::NewStub(grpc::CreateCustomChannel(
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
}

void Comm::create_streams() {
    update_streams_mu_ = std::make_unique<std::mutex[]>(Lib::NumHosts());
    pull_streams_mu_ = std::make_unique<std::mutex[]>(Lib::NumHosts());
    push_streams_mu_ = std::make_unique<std::mutex[]>(Lib::NumHosts());
    for (Hostid server = 0; server < Lib::NumHosts(); server++) {
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
}

void Comm::rpc_server_func() {
    rpc_server_->Wait();
}


void Comm::Update(Hostid server, Tableid id,
        Iteration iteration, Bytes&& bytes) {
    if (server == Lib::ThisHost()) {
        Lib::Server()->Update(Lib::ThisHost(), id, iteration, bytes);
    } else {
        rpc::UpdateRequest req;
        req.set_tableid(id);
        req.set_iteration(iteration);
        req.set_data(std::move(bytes));
        std::lock_guard<std::mutex> lock(update_streams_mu_[server]);
        update_streams_[server]->Write(req);
    }
}

void Comm::Pull(Hostid server, Tableid id, Iteration iteration) {
    if (server == Lib::ThisHost()) {
        std::thread([this, server, id, iteration] {
            auto data = Lib::Server()->GetData(Lib::ThisHost(), id, iteration);
            Lib::Client()->ServerUpdate(Lib::ThisHost(), id, std::get<0>(data), std::get<1>(data));
        }).detach();
    } else {
        rpc::PullRequest req;
        req.set_tableid(id);
        req.set_iteration(iteration);
        std::lock_guard<std::mutex> lock(pull_streams_mu_[server]);
        pull_streams_[server]->Write(req);
    }
}

void Comm::Push(Hostid client, Tableid id, Iteration iteration, Bytes&& bytes) {
    if (client == Lib::ThisHost()) {
        Lib::Client()->ServerUpdate(Lib::ThisHost(), id, iteration, bytes);
    } else {
        rpc::PushRequest req;
        req.set_tableid(id);
        req.set_data(std::move(bytes));
        req.set_iteration(iteration);
        std::lock_guard<std::mutex> lock(push_streams_mu_[client]);
        push_streams_[client]->Write(req);
    }
}

void Comm::SyncStorage(Hostid host, Tableid id, Bytes&& data) {
    rpc::SyncStorageRequest req;
    req.set_tableid(id);
    req.set_parameter(std::move(data));

    grpc::ClientContext ctx;
    rpc::SyncStorageResponse res;
    stubs_[host]->SyncStorage(&ctx, req, &res);
}



// Barrier
void Comm::Barrier() {
    std::unique_lock<std::mutex> lock(barrier_mu_);
    if (Lib::ThisHost() == 0) {
        barrier_cv_.wait(lock, [this]{return barrier_cnt_ >= Lib::NumHosts() - 1;});
        barrier_cnt_ = 0;

        for (Hostid host = 1; host < Lib::NumHosts(); ++host) {
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

void Comm::barrier_notified() {
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

void Comm::finish_handler() {
    if (!is_finish) {
        abort();
    }
}

void Comm::finish() {
    is_finish = true;
    for (Hostid host = 0; host < Lib::NumHosts(); ++host) {
        if (host != Lib::ThisHost()) {
            grpc::ClientContext ctx;
            rpc::FinishRequest req;
            rpc::FinishResponse res;
            stubs_[host]->Finish(&ctx, req, &res);
        }
    }
    std::chrono::system_clock::time_point deadline = 
        std::chrono::system_clock::now() + 1ms;
    rpc_server_->Shutdown(deadline);
    rpc_server_thread_.join();
}

Comm::~Comm() {
    finish();
}


} /* woops */ 
