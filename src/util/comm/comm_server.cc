#include "comm_server.h"
#include "lib.h"

#include <cstdlib>
#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>

#include "util/storage/dense_storage.h"
#include "util/logging.h"
#include "util/comm/comm.h"
#include "server/server.h"
#include "client/client.h"

namespace woops
{
grpc::Status CommServer::CheckAlive(
        MAYBE_UNUSED grpc::ServerContext* ctx,
        MAYBE_UNUSED const rpc::CheckAliveRequest* req,
        rpc::CheckAliveResponse* res){
    res->set_status(true);
    return grpc::Status::OK;
}

grpc::Status CommServer::Finish(
        MAYBE_UNUSED grpc::ServerContext* ctx,
        MAYBE_UNUSED const rpc::FinishRequest* req,
        MAYBE_UNUSED rpc::FinishResponse* res) {
    std::abort();
}

grpc::Status CommServer::SyncPlacement(
        MAYBE_UNUSED grpc::ServerContext* ctx,
        MAYBE_UNUSED const rpc::SyncPlacementRequest* req,
        rpc::SyncPlacementResponse* res) {
    res->set_data(Lib::Placement()->Serialize());
    return grpc::Status::OK;
}

grpc::Status CommServer::BarrierNotify(
        MAYBE_UNUSED grpc::ServerContext* ctx,
        MAYBE_UNUSED const rpc::BarrierNotifyRequest* req,
        MAYBE_UNUSED rpc::BarrierNotifyResponse* res){
    Lib::Comm()->barrier_notified_();
    return grpc::Status::OK;
}

grpc::Status CommServer::SyncStorage(
        MAYBE_UNUSED grpc::ServerContext* ctx,
        const rpc::SyncStorageRequest* req,
        MAYBE_UNUSED rpc::SyncStorageResponse* res) {
    int id = req->tableid();
    Lib::Client()->ServerSyncStorage(id, req->parameter());
    return grpc::Status::OK;
}

grpc::Status CommServer::Update(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::UpdateResponse, rpc::UpdateRequest>* stream) {
    int client = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::UpdateRequest req;
    while (stream->Read(&req)) {
        Bytes bytes(req.delta());
        Lib::Server()->Update(client, req.tableid(), bytes, req.iteration());        
    }
    return grpc::Status::OK;
}

grpc::Status CommServer::Pull(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::PullResponse, rpc::PullRequest>* stream) {
    int client = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::PullRequest req;
    std::mutex stream_mu;
    while(stream->Read(&req)) {
        std::thread t([req, &stream_mu, &stream, client] {
            int id = req.tableid();
            int iteration = req.iteration();
            Bytes parameter = Lib::Server()->GetParameter(client, id, iteration);
            Lib::Comm()->Push(client, id, std::move(parameter), iteration);
        });
        t.detach();
        
    }
    return grpc::Status::OK;
}

grpc::Status CommServer::Push(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::PushResponse, rpc::PushRequest>* stream) {
    int server = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::PushRequest req;
    while(stream->Read(&req)) {
        Bytes bytes(req.parameter());
        Lib::Client()->ServerUpdate(server, req.tableid(), bytes, req.iteration());
    }
    return grpc::Status::OK;
}
} /* woops */ 
