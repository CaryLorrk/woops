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
    client_push_streams_mu_ = std::make_unique<std::mutex[]>(Lib::NumHosts());
    client_pull_streams_mu_ = std::make_unique<std::mutex[]>(Lib::NumHosts());
    server_push_streams_mu_ = std::make_unique<std::mutex[]>(Lib::NumHosts());
    server_pull_streams_mu_ = std::make_unique<std::mutex[]>(Lib::NumHosts());
    for (Hostid server = 0; server < Lib::NumHosts(); server++) {
        auto client_push_ctx = std::make_unique<grpc::ClientContext>();
        client_push_ctx->AddMetadata("from_host", std::to_string(Lib::ThisHost()));
        client_push_streams_.push_back(stubs_[server]->ClientPush(client_push_ctx.get()));
        client_push_ctxs_.push_back(std::move(client_push_ctx));

        auto client_pull_ctx = std::make_unique<grpc::ClientContext>();
        client_pull_ctx->AddMetadata("from_host", std::to_string(Lib::ThisHost()));
        client_pull_streams_.push_back(stubs_[server]->ClientPull(client_pull_ctx.get()));
        client_pull_ctxs_.push_back(std::move(client_pull_ctx));

        auto server_push_ctx = std::make_unique<grpc::ClientContext>();
        server_push_ctx->AddMetadata("from_host", std::to_string(Lib::ThisHost()));
        server_push_streams_.push_back(stubs_[server]->ServerPush(server_push_ctx.get()));
        server_push_ctxs_.push_back(std::move(server_push_ctx));

        auto server_pull_ctx = std::make_unique<grpc::ClientContext>();
        server_pull_ctx->AddMetadata("from_host", std::to_string(Lib::ThisHost()));
        server_pull_streams_.push_back(stubs_[server]->ServerPull(server_pull_ctx.get()));
        server_pull_ctxs_.push_back(std::move(server_pull_ctx));
    }
}

void Comm::rpc_server_func() {
    rpc_server_->Wait();
}


void Comm::ClientPush(Hostid server, Tableid id,
        Iteration iteration, Bytes&& bytes) {
    if (server == Lib::ThisHost()) {
        Lib::Server()->ClientPushHandler(Lib::ThisHost(), id, iteration, bytes);
    } else {
        rpc::PushRequest req;
        req.set_tableid(id);
        req.set_iteration(iteration);
        req.set_data(std::move(bytes));
        std::lock_guard<std::mutex> lock(client_push_streams_mu_[server]);
        client_push_streams_[server]->Write(req);
    }
}

void Comm::ClientPull(Hostid server, Tableid id, Iteration iteration) {
    if (server == Lib::ThisHost()) {
        std::thread([this, server, id, iteration] {
            auto data = Lib::Server()->GetData(Lib::ThisHost(), id, iteration);
            Lib::Comm()->ServerPush(Lib::ThisHost(), id, std::get<0>(data), std::get<1>(std::move(data)));
        }).detach();
    } else {
        rpc::PullRequest req;
        req.set_tableid(id);
        req.set_iteration(iteration);
        std::lock_guard<std::mutex> lock(client_pull_streams_mu_[server]);
        client_pull_streams_[server]->Write(req);
    }
}

void Comm::ServerPush(Hostid client, Tableid id, Iteration iteration, Bytes&& bytes) {
    if (client == Lib::ThisHost()) {
        Lib::Client()->ServerPushHandler(Lib::ThisHost(), id, iteration, bytes);
    } else {
        rpc::PushRequest req;
        req.set_tableid(id);
        req.set_data(std::move(bytes));
        req.set_iteration(iteration);
        std::lock_guard<std::mutex> lock(server_push_streams_mu_[client]);
        server_push_streams_[client]->Write(req);
    }
}

void Comm::ServerPull(
        MAYBE_UNUSED Hostid client,
        MAYBE_UNUSED Tableid id,
        MAYBE_UNUSED Iteration iteration) {
    LOG(INFO) << __FUNCTION__ << "Undefined.";
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
