#include "server.h"

#include <iostream>
#include <algorithm>
#include <functional>

#include "common/context.h"
#include "common/logging.h"

using namespace std::placeholders;

namespace woops
{

void Server::Initialize() {
    Context::GetComm().RegisterMessageHandler(rpc::Message_Command_PULL_REQ,
            std::bind(&Server::HandlePullReq, this, _1, _2));
    Context::GetComm().RegisterMessageHandler(rpc::Message_Command_UPDATE,
            std::bind(&Server::HandleUpdate, this, _1, _2));
}



std::unique_ptr<ServerTable>& Server::GetTable(const std::string& tablename) {
    std::unique_lock<std::mutex> lock(tables_mu_);
    decltype(tables_.begin()) search;
    tables_cv_.wait(lock, [this, &tablename, &search] {
        search = tables_.find(tablename);
        if (search == tables_.end()) return false;
        return true;
    });
    return search->second;
    
}

void Server::Assign(const std::string& tablename, const void* parameter) {
    auto& table = GetTable(tablename);
    std::lock_guard<std::mutex> lock(table->mu);
    table->storage->Assign(parameter);
}

void Server::Update(const std::string& tablename, const void* delta, unsigned iteration) {
    auto& table = GetTable(tablename);
    auto& storage = table->storage;
    std::lock_guard<std::mutex> lock(table->mu);
    storage->Update(delta);
    unsigned host = Context::ThisHost();
    if (table->iterations[host] < iteration) {
        table->iterations[host] = iteration;
    }
    table->cv.notify_all();
}

void Server::Read(const std::string& tablename, unsigned in_iter,
        void* parameter, unsigned& out_iter) {
    auto& table = GetTable(tablename);
    std::unique_lock<std::mutex> lock(table->mu);
    table->cv.wait(lock, [this, &out_iter, &table, in_iter]{
        out_iter = *std::min_element(table->iterations.begin(), table->iterations.end()); 
        return out_iter >= in_iter - Context::Staleness();
    });
    auto p = table->storage->Serialize();
    std::copy((const int8_t*)p, (const int8_t*)p + table->storage->GetByteSize(), (int8_t*)parameter);
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

        table->iterations.resize(Context::NumHosts(), 0);
    }
    tables_cv_.notify_all();
}

void Server::HandlePullReq(unsigned host, rpc::Message msg) {
    auto& cmd_msg = msg.pull_req();
    auto& tablename = cmd_msg.tablename();
    unsigned iteration = cmd_msg.iteration();
    //LOG(INFO) << "HandlePullReq from host: " << host << " table: " << tablename << " iteration " << iteration;
    auto& table = GetTable(tablename);
    auto& storage = table->storage;
    std::unique_lock<std::mutex> lock(table->mu);
    unsigned min = *std::min_element(table->iterations.begin(), table->iterations.end());
    table->cv.wait(lock, [this, &min, &table, iteration]{
        min = *std::min_element(table->iterations.begin(), table->iterations.end()); 
        return min >= iteration - Context::Staleness();
    });
    auto pull = std::make_unique<rpc::Pull>();
    pull->set_tablename(tablename);
    pull->set_iteration(min);
    pull->set_parameter(storage->Serialize(), storage->GetByteSize());

    rpc::Message res;
    res.set_cmd(rpc::Message_Command_PULL);
    res.set_allocated_pull(pull.release());
    Context::GetComm().SendMessage(host, res);
}


void Server::HandleUpdate(unsigned host, rpc::Message msg) {
    auto& cmd_msg = msg.update();
    auto& tablename = cmd_msg.tablename();
    unsigned iteration = cmd_msg.iteration();
    //LOG(INFO) << "HandleUpdate from host: " << host << " table: " << tablename << " iteration " << iteration;
    auto& table = GetTable(tablename);
    auto& storage = table->storage;
    std::lock_guard<std::mutex> lock(table->mu);
    storage->Update(cmd_msg.delta().data());
    if (table->iterations[host] < iteration) {
        table->iterations[host] = iteration;
    }

    table->cv.notify_all();
}
    
} /* woops */ 
