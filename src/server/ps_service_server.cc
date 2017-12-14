#include "ps_service_server.h"

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
PsServiceServer::PsServiceServer(Comm* comm, Client* client, Server* server):
    comm_(comm),
    client_(client),
    server_(server) {}

grpc::Status PsServiceServer::CheckAlive(grpc::ServerContext* ctx,
        const rpc::CheckAliveRequest* req,
        rpc::CheckAliveResponse* res){
    res->set_status(true);
    return grpc::Status::OK;
}

grpc::Status PsServiceServer::BarrierNotify(grpc::ServerContext* ctx,
        const rpc::BarrierNotifyRequest* req,
        rpc::BarrierNotifyResponse* res){
    comm_->BarrierNotified();
    return grpc::Status::OK;
}

grpc::Status PsServiceServer::ForceSync(grpc::ServerContext* ctx,
        const rpc::ForceSyncRequest* req, rpc::ForceSyncResponse* res) {
    const std::string &tablename = req->tablename();
    const void *data = req->parameter().data();
    server_->Assign(tablename, data);
    return grpc::Status::OK;
}

grpc::Status PsServiceServer::Update(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::UpdateResponse, rpc::UpdateRequest>* stream) {
    int client = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::UpdateRequest req;
    while (stream->Read(&req)) {
        server_->Update(client, req.tablename(), req.delta().data(), req.iteration());        
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
            const std::string& tablename = req.tablename();
            int iteration = req.iteration();
            size_t size;
            const void* parameter = server_->GetParameter(tablename, iteration, size);
            comm_->Push(client, tablename, parameter, size, iteration);
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
        client_->ServerAssign(server, req.tablename(), req.parameter().data(), req.iteration());
    }
    return grpc::Status::OK;
}
} /* woops */ 
