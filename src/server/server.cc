#include "server.h"

#include <mutex>

namespace woops
{

void Server::Initialize(const WoopsConfig& config, Comm *comm) {
    comm_ = comm;

    this_host_ = config.this_host;
    staleness_ = config.staleness;
    num_hosts_ = config.hosts.size();
}

void Server::CreateTable(const TableConfig& config, size_t size) {
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

void Server::LocalAssign(const std::string& name, const void* data) {
    auto& table = GetTable(name);
    std::lock_guard<std::mutex> lock(table->mu);
    table->storage->Assign(data);
}

void Server::LocalUpdate(const std::string& name, const void* delta, int iteration) {
    auto& table = GetTable(name);
    std::lock_guard<std::mutex> lock(table->mu);
    table->storage->Update(delta);
    table->iterations[this_host_] = iteration;
    int min = *std::min_element(table->iterations.begin(), table->iterations.end());
    if (min >= iteration - staleness_) {
        table->cv.notify_all();
    }
}

std::unique_ptr<ServerTable>& Server::GetTable(const std::string& name) {
    std::unique_lock<std::mutex> lock(tables_mu_);
    decltype(tables_.begin()) search;
    tables_cv_.wait(lock, [this, &name, &search] {
        search = tables_.find(name);
        if (search == tables_.end()) return false;
        return true;
    });
    return search->second;
    
}

std::string Server::ToString() {
    std::stringstream ss;
    for(auto& i: tables_) {
        ss << i.first << ": " << i.second->storage->ToString() << std::endl; 
    }
    return ss.str();
}


 
} /* woops */ 
