#include "client.h"

#include <algorithm>

#include "util/logging.h"
#include "util/comm/comm.h"
#include "util/placement/placement.h"

namespace woops
{ 


void Client::Initialize(const WoopsConfig& config, Comm* comm, Placement* placement) {
    placement_ = placement;
    comm_ = comm;

    this_host_ = config.this_host;
    staleness_ = config.staleness;
    port_ = config.port;
    hosts_ = config.hosts;
}

void Client::CreateTable(const TableConfig& config) {
    auto pair = tables_.emplace(config.id, std::make_unique<ClientTable>());
    auto& table = pair.first->second;
    table->size = config.size;
    table->element_size = config.element_size;
    table->cache = config.cache_constructor(table->size);
    table->config = config;
    placement_->RegisterTable(config);
}


void Client::LocalAssign(Tableid id, const void* data) {
    auto& table = tables_[id];
    table->cache->Assign(data);
}

void Client::ServerAssign(Hostid server, Tableid id, const void* data, int iteration) {
    auto& table = tables_[id];
    auto& partition = placement_->GetPartitions(id)[server];
    {
        std::lock_guard<std::mutex> lock(table->mu);
        table->cache->Assign(data, partition.begin, partition.end - partition.begin);
        if (table->iterations[server] < iteration) {
            table->iterations[server] = iteration;
        }
    }
    table->cv.notify_all();
}

void Client::Update(Tableid id, Storage& data) {
    auto& table = tables_[id];
    auto& partitions = placement_->GetPartitions(id);
    auto server_to_bytes = data.Encoding(partitions);
    for (auto& kv: server_to_bytes) {
        auto& server = kv.first;
        auto& bytes = kv.second;
        comm_->Update(server, id, bytes, iteration_);
    }
    int min = std::min_element(
            table->iterations.begin(), table->iterations.end(),
            [](ClientTable::Iterations::value_type& l,
                ClientTable::Iterations::value_type& r) -> bool {
                return l.second < r.second;
            })->second;
    if (min < iteration_ - staleness_) {
        for (auto& kv: partitions) {
            comm_->Pull(kv.first, id, iteration_);
        }
    }
}

void Client::Clock() {
    iteration_++;    
}

void Client::Sync(Tableid id) {
    auto& table = tables_[id];
    std::unique_lock<std::mutex> lock(table->mu);
    table->cv.wait(lock, [this, &table]{
            int min = std::min_element(
                    table->iterations.begin(), table->iterations.end(),
                    [](ClientTable::Iterations::value_type& l,
                        ClientTable::Iterations::value_type& r) -> bool {
                        return l.second < r.second;
                    })->second;
            return min >= iteration_ - staleness_ - 1;
    });
}


void Client::ForceSync() {
    LOG(INFO) << "ForceSync";
    if (this_host_ == 0) {
        placement_->Decision();
    }
    comm_->Barrier();
    if (this_host_ != 0) {
        comm_->SyncPlacement();
    }
    for (auto& kv: tables_) {
        Tableid tableid = kv.first;
        auto& table = kv.second;
        auto& partitions = placement_->GetPartitions(tableid);
        for (auto& kv: partitions) {
            Hostid server= kv.first;
            Placement::Partition& partition = kv.second;
            if (server == this_host_) {
                auto begin = partition.begin;
                auto end = partition.end;
                comm_->CreateTable(table->config, end - begin);
            }
            table->iterations[server] = -1;
        }
    }
    comm_->Barrier();
    if (this_host_ == 0) {
        for (auto& kv: tables_) {
            Tableid tableid = kv.first;
            auto& table = kv.second;
            auto server_to_bytes = table->cache->Encoding(placement_->GetPartitions(tableid));
            for (auto& kv: server_to_bytes) {
                auto& server = kv.first;
                auto& bytes = kv.second;
                comm_->ForceSync(server, tableid, bytes);
            }
        }
    }
    comm_->Barrier();
    LOG(INFO) << "ForceSync End";
}

std::string Client::ToString() {
    std::stringstream ss;
    ss << "Client: " << std::endl;    
    for (auto& pair: tables_) {
        ss << pair.first << ": " << pair.second->cache->ToString() << std::endl;
    }
    return ss.str();
}

Client::Client():
    iteration_(0){}

} /* woops */ 
