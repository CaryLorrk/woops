#include "server.h"

#include <mutex>

namespace woops
{

void Server::Initialize(const WoopsConfig& config, Comm* comm) {
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
    tables_cv_.notify_one();
}

void Server::Assign(const std::string& tablename, const void* data) {
    auto& table = tables_[tablename];
    std::lock_guard<std::mutex> lock(table->mu);
    table->storage->Assign(data);
}

void Server::Update(int client, const std::string& tablename, const void* delta, int iteration) {
    auto& table = tables_[tablename];
    std::lock_guard<std::mutex> lock(table->mu);
    table->storage->Update(delta);
    if (table->iterations[client] < iteration) {
        table->iterations[client] = iteration;
    }
    int min = *std::min_element(table->iterations.begin(), table->iterations.end());
    if (min >= iteration - staleness_) {
        table->cv.notify_all();
    }
}

const void* Server::GetParameter(int client, const std::string& tablename, int &iteration, size_t &size) {
    auto& table = tables_[tablename];
    auto& storage = table->storage;
    int min;
    std::unique_lock<std::mutex> lock(table->mu); 
    table->cv.wait(lock, [this, &table, iteration, &min]{
        min = *std::min_element(table->iterations.begin(), table->iterations.end());
        return min >= iteration - staleness_;
    });
    iteration = min;
    size = storage->GetSize();
    return storage->Serialize();
}

std::string Server::ToString() {
    std::stringstream ss;
    for(auto& i: tables_) {
        ss << i.first << ": " << i.second->storage->ToString() << std::endl; 
    }
    return ss.str();
}


 
} /* woops */ 
