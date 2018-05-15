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
    for (Hostid host = 0; host < Lib::NumHosts(); ++host) {
        table->storages[host] = std::unique_ptr<Storage>(config.server_storage_constructor());
    }

    table->size = size;
    table->element_size = config.element_size;

    table->iterations.resize(Lib::NumHosts(), -1);
}

void Server::Update(Hostid client, Tableid id, Iteration iteration, const Bytes& bytes) {
    auto&& table = tables_[id];
    std::lock_guard<std::mutex> lock(table->mu);
    for (Hostid host = 0; host < Lib::NumHosts(); ++host) {
        table->storages[host]->Decode(host, bytes);
    }
    if (table->iterations[client] < iteration) {
        table->iterations[client] = iteration;
    }
    Iteration min = *std::min_element(table->iterations.begin(), table->iterations.end());
    if (min >= iteration - Lib::Staleness()) {
        table->cv.notify_all();
    }
}

std::tuple<Iteration, Bytes> Server::GetData(Hostid client, Tableid id, Iteration iteration) {
    auto&& table = tables_[id];
    auto&& storage = table->storages[client];
    Iteration min;
    std::unique_lock<std::mutex> lock(table->mu); 
    table->cv.wait(lock, [&table, iteration, &min]{
        min = *std::min_element(table->iterations.begin(), table->iterations.end());
        return min >= iteration - Lib::Staleness();
    });
    return make_tuple(min, storage->Encode());
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
