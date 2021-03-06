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
    Lib::Comm().finish_handler();
    return grpc::Status::OK;
}

grpc::Status CommServer::SyncPlacement(
        MAYBE_UNUSED grpc::ServerContext* ctx,
        MAYBE_UNUSED const rpc::SyncPlacementRequest* req,
        rpc::SyncPlacementResponse* res) {
    res->set_data(Lib::Placement().Serialize());
    return grpc::Status::OK;
}

grpc::Status CommServer::BarrierNotify(
        MAYBE_UNUSED grpc::ServerContext* ctx,
        MAYBE_UNUSED const rpc::BarrierNotifyRequest* req,
        MAYBE_UNUSED rpc::BarrierNotifyResponse* res){
    Lib::Comm().barrier_notified();
    return grpc::Status::OK;
}

grpc::Status CommServer::SyncStorage(
        MAYBE_UNUSED grpc::ServerContext* ctx,
        const rpc::SyncStorageRequest* req,
        MAYBE_UNUSED rpc::SyncStorageResponse* res) {
    Lib::Client().SyncStorageHandler(req->tableid(), req->parameter());
    return grpc::Status::OK;
}

grpc::Status CommServer::ClientPush(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::PushResponse, rpc::PushRequest>* stream) {
    Hostid client = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::PushRequest req;
    while (stream->Read(&req)) {
        Lib::Server().ClientPushHandler(client, req.tableid(), req.iteration(), Bytes(std::move(req.data())));        
    }
    return grpc::Status::OK;
}

grpc::Status CommServer::ClientPull(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::PullResponse, rpc::PullRequest>* stream) {
    Hostid client = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::PullRequest req;
    while(stream->Read(&req)) {
        std::thread([req, client] {
            Hostid id = req.tableid();
            auto data = Lib::Server().GetData(client, id, req.iteration());
            Lib::Comm().ServerPush(client, id, std::get<0>(data), std::move(std::get<1>(data)));
        }).detach();
        
    }
    return grpc::Status::OK;
}

grpc::Status CommServer::ServerPush(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::PushResponse, rpc::PushRequest>* stream) {
    Hostid server = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::PushRequest req;
    while(stream->Read(&req)) {
        Lib::Client().ServerPushHandler(server, req.tableid(), req.iteration(), Bytes(std::move(req.data())));
    }
    return grpc::Status::OK;
}

grpc::Status CommServer::ServerPull(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::PullResponse, rpc::PullRequest>* stream) {
    Hostid server = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::PullRequest req;
    while(stream->Read(&req)) {
        std::thread([req, server] {
            LOG(INFO) << __FUNCTION__ << "Undefined.";
        }).detach();
        
    }
    return grpc::Status::OK;
}
} /* woops */ 
