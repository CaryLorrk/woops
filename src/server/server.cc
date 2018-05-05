#include "server.h"

#include <mutex>

#include "lib.h"

namespace woops
{

void Server::CreateTable(const TableConfig& config, size_t size) {
    std::lock_guard<std::mutex> lock(tables_mu_);
    auto&& pair = tables_.emplace(config.id, std::make_unique<ServerTable>());
    auto&& table = pair.first->second;

    table->storages.resize(Lib::NumHosts());
    for (int i = 0; i < Lib::NumHosts(); ++i) {
        table->storages[i] = config.server_storage_constructor();
    }

    table->size = size;
    table->element_size = config.element_size;

    table->iterations.resize(Lib::NumHosts(), -1);
}

void Server::Update(Hostid client, Tableid id, const Bytes& bytes, Iteration iteration) {
    auto&& table = tables_[id];
    std::lock_guard<std::mutex> lock(table->mu);
    for (int i = 0; i < Lib::NumHosts(); ++i) {
        if (client == Lib::ThisHost()) continue;
        table->storages[i]->Decode(bytes);
    }
    if (table->iterations[client] < iteration) {
        table->iterations[client] = iteration;
    }
    int min = *std::min_element(table->iterations.begin(), table->iterations.end());
    if (min >= iteration - Lib::Staleness()) {
        table->cv.notify_all();
    }
}

Bytes Server::GetParameter(Hostid client, Tableid id, Iteration& iteration) {
    auto&& table = tables_[id];
    auto&& storage = table->storages[client];
    Iteration min;
    std::unique_lock<std::mutex> lock(table->mu); 
    table->cv.wait(lock, [this, &table, iteration, &min]{
        min = *std::min_element(table->iterations.begin(), table->iterations.end());
        return min >= iteration - Lib::Staleness();
    });
    iteration = min;
    Bytes ret = storage->Encode();
    storage->Zerofy();
    return ret;
}

std::string Server::ToString() {
    std::stringstream ss;
    for(auto&& kv: tables_) {
        ss << kv.first << ": \n";
        auto&& table = kv.second;
        for (size_t client = 0; client < table->storages.size(); ++client) {
            ss << "client: " << client << " iteration: " << table->iterations[client] << "\n";
            ss << kv.second->storages[client]->ToString() << std::endl; 
        }
    }
    return ss.str();
}


 
} /* woops */ 
