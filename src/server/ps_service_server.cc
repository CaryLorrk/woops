#include "ps_service_server.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>
#include "common/storage/dense_storage.h"
#include "common/logging.h"

namespace woops
{
PsServiceServer::PsServiceServer(const size_t num_hosts, const int staleness):
    num_hosts_(num_hosts),
    staleness_(staleness)
{
        
}

grpc::Status PsServiceServer::CheckAlive(grpc::ServerContext* ctx,
        const rpc::CheckAliveRequest* req,
        rpc::CheckAliveResponse* res){
    res->set_status(true);
    return grpc::Status::OK;
}

grpc::Status PsServiceServer::Assign(grpc::ServerContext* ctx,
        const rpc::AssignRequest* req, rpc::AssignResponse* res) {
    auto& table = GetTable(req->name());
    auto& storage = table->storage;
    if (req->client() == 0) {
        storage->Assign(req->param().data());
    }
    
    std::unique_lock<std::mutex> lock(table->mu);
    table->assign_cnt++;
    table->cv.notify_all();
    table->cv.wait(lock, [this, &table, req] {
        return table->assign_cnt % num_hosts_ == 0;
    });
    if (req->client() != 0) {
        res->set_param(storage->Serialize(), storage->GetSize());
    }
    return grpc::Status::OK;

}

grpc::Status PsServiceServer::Update(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::UpdateResponse, rpc::UpdateRequest>* stream) {
    int client = std::stoi(ctx->client_metadata().find("client")->second.data());
    rpc::UpdateRequest req;
    while (stream->Read(&req)) {
        auto& table = GetTable(req.name());
        auto& storage = table->storage;
        std::unique_lock<std::mutex> lock(table->mu); 
        storage->Update(req.delta().data());
        auto& iteration = table->iterations[client];
        if (iteration < req.iteration()) {
            iteration = req.iteration();
        }
        int min = *std::min_element(table->iterations.begin(), table->iterations.end());
        if (min >= iteration - staleness_) {
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
            auto& table = GetTable(req.name());
            auto& name = req.name();
            auto& storage = table->storage;
            int min;
            {
                std::unique_lock<std::mutex> lock(table->mu); 
                auto iteration = req.iteration();
                table->cv.wait(lock, [this, &table, iteration, &min]{
                    min = *std::min_element(table->iterations.begin(), table->iterations.end());
                    return min >= iteration - staleness_ - 1;
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

std::unique_ptr<ServerTable>& PsServiceServer::GetTable(const std::string& name) {
    std::unique_lock<std::mutex> lock(tables_mu_);
    decltype(tables_.begin()) search;
    tables_cv_.wait(lock, [this, &name, &search] {
        search = tables_.find(name);
        if (search == tables_.end()) return false;
        return true;
    });
    return search->second;
    
}

void PsServiceServer::LocalAssign(const std::string& name, const void* data) {
    auto& table = GetTable(name);
    std::lock_guard<std::mutex> lock(table->mu);
    table->storage->Assign(data);
}

void PsServiceServer::LocalUpdate(const std::string& name, const void* delta, const int iteration, int this_host) {
    auto& table = GetTable(name);
    std::lock_guard<std::mutex> lock(table->mu);
    table->storage->Update(delta);
    table->iterations[this_host] = iteration;
    int min = *std::min_element(table->iterations.begin(), table->iterations.end());
    if (min >= iteration - staleness_) {
        table->cv.notify_all();
    }
}

void PsServiceServer::CreateTable(const TableConfig& config, size_t size) {
    {
        std::lock_guard<std::mutex> lock(tables_mu_);
        auto pair = tables_.emplace(config.name, std::make_unique<ServerTable>());
        auto& table = pair.first->second;

        table->storage = config.server_storage_constructor(size);
        table->storage->Zerofy();
        table->size = size;
        table->element_size = config.element_size;
        table->assign_cnt = 0;

        table->iterations.resize(num_hosts_, -1);
    }
    tables_cv_.notify_all();
}

std::string PsServiceServer::ToString() {
    std::stringstream ss;
    for(auto& i: tables_) {
        ss << i.first << ": " << i.second->storage->ToString() << std::endl; 
    }
    return ss.str();
}
    
} /* woops */ 
