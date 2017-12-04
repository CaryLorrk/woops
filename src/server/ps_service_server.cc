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
PsServiceServer::PsServiceServer(Comm *comm, Client *client, Server *server):
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

grpc::Status PsServiceServer::Assign(grpc::ServerContext* ctx,
        const rpc::AssignRequest* req, rpc::AssignResponse* res) {
    const std::string &tablename = req->tablename();
    const void *data = req->parameter().data();
    server_->LocalAssign(tablename, data);
    return grpc::Status::OK;
}

grpc::Status PsServiceServer::Update(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::UpdateResponse, rpc::UpdateRequest>* stream) {
    int client = std::stoi(ctx->client_metadata().find("client")->second.data());
    rpc::UpdateRequest req;
    while (stream->Read(&req)) {
        auto& table = server_->GetTable(req.name());
        auto& storage = table->storage;
        std::unique_lock<std::mutex> lock(table->mu); 
        storage->Update(req.delta().data());
        auto& iteration = table->iterations[client];
        if (iteration < req.iteration()) {
            iteration = req.iteration();
        }
        int min = *std::min_element(table->iterations.begin(), table->iterations.end());
        if (min >= iteration - server_->staleness_) {
            table->cv.notify_all();
        }
    }
    return grpc::Status::OK;
}

grpc::Status PsServiceServer::Pull(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::PullResponse, rpc::PullRequest>* stream) {
    int client = std::stoi(ctx->client_metadata().find("client")->second.data());
    rpc::PullRequest req;
    std::mutex stream_mu;
    while(stream->Read(&req)) {
        std::thread t([this, req, &stream_mu, &stream, client] {
            auto& table = server_->GetTable(req.name());
            auto& name = req.name();
            auto& storage = table->storage;
            int min;
            {
                std::unique_lock<std::mutex> lock(table->mu); 
                auto iteration = req.iteration();
                table->cv.wait(lock, [this, &table, iteration, &min]{
                    min = *std::min_element(table->iterations.begin(), table->iterations.end());
                    return min >= iteration - server_->staleness_ - 1;
                });
            }
            rpc::PullResponse res;
            res.set_name(name);
            res.set_iteration(min);
            res.set_param(storage->Serialize(), storage->GetSize());
            {
                std::lock_guard<std::mutex> lock(stream_mu);
                stream->Write(res);
            }
        });
        t.detach();
        
    }
    return grpc::Status::OK;
}


    
} /* woops */ 
