#include "ps_service_server.h"
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
grpc::Status PsServiceServer::CheckAlive(grpc::ServerContext* ctx,
        const rpc::CheckAliveRequest* req,
        rpc::CheckAliveResponse* res){
    res->set_status(true);
    return grpc::Status::OK;
}

grpc::Status PsServiceServer::Finish(grpc::ServerContext* ctx,
        const rpc::FinishRequest* req, rpc::FinishResponse* res) {
    std::abort();
}

grpc::Status PsServiceServer::SyncPlacement(grpc::ServerContext* ctx,
        const rpc::SyncPlacementRequest* req, rpc::SyncPlacementResponse* res) {
    res->set_data(Lib::Placement()->Serialize());
    return grpc::Status::OK;
}

grpc::Status PsServiceServer::BarrierNotify(grpc::ServerContext* ctx,
        const rpc::BarrierNotifyRequest* req,
        rpc::BarrierNotifyResponse* res){
    Lib::Comm()->barrier_notified_();
    return grpc::Status::OK;
}

grpc::Status PsServiceServer::ForceSync(grpc::ServerContext* ctx,
        const rpc::ForceSyncRequest* req, rpc::ForceSyncResponse* res) {
    int id = req->tableid();
    const void *data = req->parameter().data();
    Lib::Server()->Assign(id, data);
    return grpc::Status::OK;
}

grpc::Status PsServiceServer::Update(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::UpdateResponse, rpc::UpdateRequest>* stream) {
    int client = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::UpdateRequest req;
    while (stream->Read(&req)) {
        Lib::Server()->Update(client, req.tableid(), req.delta().data(), req.iteration());        
    }
    return grpc::Status::OK;
}

grpc::Status PsServiceServer::Pull(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::PullResponse, rpc::PullRequest>* stream) {
    int client = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::PullRequest req;
    std::mutex stream_mu;
    while(stream->Read(&req)) {
        std::thread t([this, req, &stream_mu, &stream, client] {
            int id = req.tableid();
            int iteration = req.iteration();
            size_t size;
            const void* parameter = Lib::Server()->GetParameter(id, iteration, size);
            Lib::Comm()->Push(client, id, parameter, size, iteration);
        });
        t.detach();
        
    }
    return grpc::Status::OK;
}

grpc::Status PsServiceServer::Push(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::PushResponse, rpc::PushRequest>* stream) {
    int server = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::PushRequest req;
    while(stream->Read(&req)) {
        Lib::Client()->ServerAssign(server, req.tableid(), req.parameter().data(), req.iteration());
    }
    return grpc::Status::OK;
}
} /* woops */ 
