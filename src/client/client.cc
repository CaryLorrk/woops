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
    auto&& table = *pair.first->second;
    table.size = config.size;
    table.element_size = config.element_size;
    table.worker_storage = std::unique_ptr<Storage>(config.worker_storage_constructor(config.id));
    table.config = config;
    table.apply_buffer = std::unique_ptr<Storage>(config.apply_buffer_constructor(config.id));
    table.need_apply = false;
    table.transmit_buffer = std::unique_ptr<Storage>(config.transmit_buffer_constructor(config.id));
}

void Client::LocalAssign(Tableid id, const Storage& data) {
    auto&& table = GetTable(id);
    std::lock_guard<std::mutex> lock(table.mu);
    table.worker_storage->Assign(data);
}

void Client::Update(Tableid id, const Storage& data) {
    Lib::Consistency().BeforeClientUpdate(id, data, iteration_);
    auto&& table = GetTable(id);
    {
        std::lock_guard<std::mutex> lock(table.mu);
        table.transmit_buffer->Update(data);
    }
    Lib::Consistency().AfterClientUpdate(id, data, iteration_);
}

void Client::Clock() {
    Lib::Consistency().BeforeClock(iteration_);
    iteration_++;    

}

void Client::Sync(Tableid id) {
    Lib::Consistency().ClientSync(id, iteration_);
}


void Client::Start() {
    LOG(INFO) << "Starting Parameter Server";
    sync_placement();
    sync_server();
    Lib::Comm().Barrier();
    sync_client();
    Lib::Comm().Barrier();
    Lib::Consistency().Start();
}

std::string Client::ToString() {
    std::stringstream ss;
    ss << "Client: " << std::endl;    
    for (auto&& pair: tables_) {
        ss << pair.first << ": " << pair.second->worker_storage->ToString() << std::endl;
    }
    return ss.str();
}

ClientTable& Client::GetTable(Tableid id) {
    return *tables_[id].get();
}

std::map<Tableid, std::unique_ptr<ClientTable>>& Client::GetTables() {
    return tables_;
}

void Client::SyncStorageHandler(Tableid id, const Bytes& bytes) {
    auto&& table = GetTable(id);
    {
        std::lock_guard<std::mutex> lock(table.mu);
        table.worker_storage->Deserialize(bytes);
    }
    table.cv.notify_all();
}

void Client::ServerPushHandler(Hostid server, Tableid id, Iteration iteration, const Bytes& bytes) {
    Lib::Consistency().BeforeServerPushHandler(server, id, iteration, bytes, iteration_);
    auto&& table = GetTable(id);
    {
        std::lock_guard<std::mutex> lock(table.mu);
        table.apply_buffer->Decode(bytes, Lib::Placement().GetPartitions(id).at(server));
        table.need_apply = true;
        if (table.iterations[server] < iteration) {
            table.iterations[server] = iteration;
        }
    }
    Lib::Consistency().AfterServerPushHandler(server, id, iteration, bytes, iteration_);
}

void Client::sync_placement() {
    if (Lib::ThisHost() == 0) {
        Lib::Placement().Decision();
    }
    Lib::Comm().Barrier();
    if (Lib::ThisHost() != 0) {
        Lib::Comm().SyncPlacement();
    }
}

void Client::sync_server() {
    for (auto&& kv: tables_) {
        Tableid id = kv.first;
        auto&& table = *kv.second;
        auto&& partitions = Lib::Placement().GetPartitions(id);
        for (auto&& kv: partitions) {
            Hostid server= kv.first;
            const Placement::Partition& partition = kv.second;
            if (server == Lib::ThisHost()) {
                auto begin = partition.begin;
                auto end = partition.end;
                Lib::Server().CreateTable(table.config, end - begin);
            }
            table.iterations[server] = -1;
        }
    }
}

void Client::sync_client() {
    if (Lib::ThisHost() == 0) {
        for (auto&& kv: tables_) {
            Tableid id = kv.first;
            auto&& table = *kv.second;
            auto bytes = table.worker_storage->Serialize();
            for (Hostid hostid = 1; hostid < Lib::NumHosts(); ++hostid) {
                Lib::Comm().SyncStorage(hostid, id, Bytes(bytes));
            }
        }
    }
}

} /* woops */ 
