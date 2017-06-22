#include "ps_service_server.h"

#include <algorithm>
#include <memory>
#include <mutex>
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
    res->set_param(storage->Serialize(), storage->GetSize());
    return grpc::Status::OK;

}

grpc::Status PsServiceServer::Update(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::UpdateResponse, rpc::UpdateRequest>* stream) {
    int client = std::stoi(ctx->client_metadata().find("client")->second.data());
    rpc::UpdateRequest req;
    while (stream->Read(&req)) {
        auto& name = req.name();
        auto& table = GetTable(req.name());
        auto& storage = table->storage;
        std::unique_lock<std::mutex> lock(table->mu); 
        storage->Update(req.delta().data());
        auto& iteration = table->iterations[client];
        iteration += 1;
        table->cv.notify_all();
        int min;
        table->cv.wait(lock, [this, &table, iteration, &min]{
            min = *std::min_element(table->iterations.begin(), table->iterations.end());
            return min >= iteration - staleness_;
        });
        rpc::UpdateResponse res;
        res.set_name(name);
        res.set_iteration(min);
        res.set_param(storage->Serialize(), storage->GetSize());
        stream->Write(res);
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
    std::unique_lock<std::mutex> lock(table->mu);
    table->storage->Assign(data);
}

void PsServiceServer::CreateTable(const TableConfig& config, size_t size) {
    std::unique_lock<std::mutex> lock(tables_mu_);
    auto pair = tables_.emplace(config.name, std::make_unique<ServerTable>());
    auto& table = pair.first->second;

    table->storage = config.server_storage_constructor(size);
    table->storage->Zerofy();
    table->size = size;
    table->element_size = config.element_size;
    table->assign_cnt = 0;

    table->iterations.resize(num_hosts_, -1);
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
