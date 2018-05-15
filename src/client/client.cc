#include "client.h"

#include <algorithm>

#include "util/logging.h"
#include "util/placement/placement.h"
#include "util/comm/comm.h"
#include "lib.h"

namespace woops
{ 

Client::Client():
    iteration_(0){}

void Client::CreateTable(const TableConfig& config) {
    auto&& pair = tables_.emplace(config.id, std::make_unique<ClientTable>());
    auto&& table = pair.first->second;
    table->size = config.size;
    table->element_size = config.element_size;
    table->storage = std::unique_ptr<Storage>(config.client_storage_constructor());
    table->config = config;
    table->apply_buffer = std::unique_ptr<Storage>(config.apply_buffer_constructor());
    table->need_apply = false;
    table->transmit_buffer = std::unique_ptr<Storage>(config.transmit_buffer_constructor());
}

void Client::LocalAssign(Tableid id, const Storage& data) {
    auto&& table = tables_[id];
    std::lock_guard<std::mutex> lock(table->mu);
    table->storage->Assign(data);
}

void Client::Update(Tableid id, const Storage& data) {
    auto&& table = tables_[id];
    {
        std::lock_guard<std::mutex> lock(table->mu);
        table->transmit_buffer->Update(data);
        if (table->need_apply) {
            table->storage->Update(*table->apply_buffer);
            table->need_apply = false;
            table->apply_buffer->Zerofy();
        }
    }

    {
        auto&& partitions = Lib::Placement()->GetPartitions(id);
        std::lock_guard<std::mutex> lock(table->mu);
        auto server_to_bytes = table->transmit_buffer->Encode(partitions);
        for (auto&& kv: server_to_bytes) {
            auto&& server = kv.first;
            auto&& bytes = kv.second;
            Lib::Comm()->Update(server, id, iteration_, std::move(bytes));
        }

        int min = std::min_element(
                table->iterations.begin(), table->iterations.end(),
                [](ClientTable::Iterations::value_type& l,
                    ClientTable::Iterations::value_type& r) -> bool {
                    return l.second < r.second;
                })->second;
        if (min < iteration_ - Lib::Staleness()) {
            for (auto&& kv: partitions) {
                Lib::Comm()->Pull(kv.first, id, iteration_);
            }
        }
    }
}

void Client::Clock() {
    iteration_++;    
}

void Client::Sync(Tableid id) {
    auto&& table = tables_[id];
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


void Client::Start() {
    LOG(INFO) << "Starting Parameter Server";
    sync_placement();
    sync_server();
    Lib::Comm()->Barrier();
    sync_client();
    Lib::Comm()->Barrier();
}

std::string Client::ToString() {
    std::stringstream ss;
    ss << "Client: " << std::endl;    
    for (auto&& pair: tables_) {
        ss << pair.first << ": " << pair.second->storage->ToString() << std::endl;
    }
    return ss.str();
}

void Client::ServerSyncStorage(Tableid id, const Bytes& bytes) {
    auto&& table = tables_[id];
    {
        std::lock_guard<std::mutex> lock(table->mu);
        table->storage->Deserialize(bytes);
    }
    table->cv.notify_all();
}

void Client::ServerUpdate(Hostid server, Tableid id, Iteration iteration, const Bytes& bytes) {
    auto&& table = tables_[id];
    {
        std::lock_guard<std::mutex> lock(table->mu);
        table->apply_buffer->Decode(bytes, Lib::Placement()->GetPartitions(id).at(server));
        table->need_apply = true;
        if (table->iterations[server] < iteration) {
            table->iterations[server] = iteration;
        }
    }
    table->cv.notify_all();
}

void Client::sync_placement() {
    if (Lib::ThisHost() == 0) {
        Lib::Placement()->Decision();
    }
    Lib::Comm()->Barrier();
    if (Lib::ThisHost() != 0) {
        Lib::Comm()->SyncPlacement();
    }
}

void Client::sync_server() {
    for (auto&& kv: tables_) {
        Tableid id = kv.first;
        auto&& table = kv.second;
        auto&& partitions = Lib::Placement()->GetPartitions(id);
        for (auto&& kv: partitions) {
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

void Client::sync_client() {
    if (Lib::ThisHost() == 0) {
        for (auto&& kv: tables_) {
            Tableid id = kv.first;
            auto&& table = kv.second;
            auto bytes = table->storage->Serialize();
            for (Hostid hostid = 1; hostid < Lib::NumHosts(); ++hostid) {
                Lib::Comm()->SyncStorage(hostid, id, Bytes(bytes));
            }
        }
    }
}

} /* woops */ 
