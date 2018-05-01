#include "client.h"

#include <algorithm>

#include "util/logging.h"
#include "util/placement/placement.h"
#include "util/comm/comm.h"
#include "lib.h"

namespace woops
{ 


void Client::CreateTable(const TableConfig& config) {
    auto pair = tables_.emplace(config.id, std::make_unique<ClientTable>());
    auto& table = pair.first->second;
    table->size = config.size;
    table->element_size = config.element_size;
    table->cache = config.cache_constructor();
    table->config = config;
}

void Client::Assign(Tableid tableid, const Bytes& bytes) {
    for (Hostid hostid = 0; hostid < Lib::NumHosts(); ++hostid) {
        Lib::Comm()->Assign(hostid, tableid, bytes);
    }
}

void Client::LocalAssign(Tableid id, const Bytes& bytes) {
    auto& table = tables_[id];
    table->cache->Assign(bytes);
}

void Client::ServerAssign(Hostid server, Tableid id, const Bytes& bytes, int iteration) {
    auto& table = tables_[id];
    auto& partition = Lib::Placement()->GetPartitions(id).at(server);
    {
        std::lock_guard<std::mutex> lock(table->mu);
        table->cache->Decode(bytes, partition.begin, Storage::DecodingType::ASSIGN);
        table->iterations[server] = iteration;
    }
    table->cv.notify_all();
}

void Client::ServerUpdate(Hostid server, Tableid id, const Bytes& bytes, int iteration) {
    auto& table = tables_[id];
    auto& partition = Lib::Placement()->GetPartitions(id).at(server);
    {
        std::lock_guard<std::mutex> lock(table->mu);
        table->cache->Decode(bytes, partition.begin, Storage::DecodingType::UPDATE);
        if (table->iterations[server] < iteration) {
            table->iterations[server] = iteration;
        }
    }
    table->cv.notify_all();
}

void Client::Update(Tableid tableid, const Storage& data) {
    auto& table = tables_[tableid];
    auto& partitions = Lib::Placement()->GetPartitions(tableid);
    auto server_to_bytes = data.Encode(partitions);
    for (auto& kv: server_to_bytes) {
        auto& server = kv.first;
        auto& bytes = kv.second;
        Lib::Comm()->Update(server, tableid, bytes, iteration_);
    }
    int min = std::min_element(
            table->iterations.begin(), table->iterations.end(),
            [](ClientTable::Iterations::value_type& l,
                ClientTable::Iterations::value_type& r) -> bool {
                return l.second < r.second;
            })->second;
    if (min < iteration_ - Lib::Staleness()) {
        for (auto& kv: partitions) {
            Lib::Comm()->Pull(kv.first, tableid, iteration_);
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
            return min >= iteration_ - Lib::Staleness() - 1;
    });
}


void Client::SyncPlacement() {
    if (Lib::ThisHost() == 0) {
        Lib::Placement()->Decision();
    }
    Lib::Comm()->Barrier();
    if (Lib::ThisHost() != 0) {
        Lib::Comm()->SyncPlacement();
    }
}

void Client::SyncServer() {
    for (auto& kv: tables_) {
        Tableid tableid = kv.first;
        auto& table = kv.second;
        auto& partitions = Lib::Placement()->GetPartitions(tableid);
        for (auto& kv: partitions) {
            Hostid server= kv.first;
            const Placement::Partition& partition = kv.second;
            if (server == Lib::ThisHost()) {
                auto begin = partition.begin;
                auto end = partition.end;
                Lib::Server()->CreateTable(table->config, end - begin);
            }
            table->iterations[server] = -1;
        }
    }
}

void Client::SyncClient() {
    if (Lib::ThisHost() == 0) {
        for (auto& kv: tables_) {
            Tableid tableid = kv.first;
            auto& table = kv.second;
            auto bytes = table->cache->Encode();
            Assign(tableid, std::move(bytes));
        }
    }
}
void Client::Start() {
    LOG(INFO) << "ForceSync";
    SyncPlacement();
    SyncServer();
    Lib::Comm()->Barrier();
    SyncClient();
    Lib::Comm()->Barrier();
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
