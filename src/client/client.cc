#include "client.h"

#include <algorithm>

#include "util/logging.h"
#include "util/comm/comm.h"

namespace woops
{ 


void Client::Initialize(const WoopsConfig& config, Comm* comm) {
    comm_ = comm;

    this_host_ = config.this_host;
    staleness_ = config.staleness;
    port_ = config.port;
    hosts_ = config.hosts;
}

void Client::CreateTable(const TableConfig& config) {
    auto pair = tables_.emplace(config.name, std::make_unique<Table>());
    auto& table = pair.first->second;
    table->size = config.size;
    table->element_size = config.element_size;
    table->cache = config.cache_constructor(table->size);


    /* split table */
    int div = table->size / hosts_.size();
    int mod = table->size % hosts_.size();
    int idx = 0;
    auto& ends = table->host_ends;
    for (size_t i = 0; i < hosts_.size(); ++i) {
        idx += i < (hosts_.size() - mod) ? div : div+1;
        ends.push_back(idx);
    }
    int start = this_host_ == 0 ? 0 : ends[this_host_-1];
    int end = ends[this_host_];


    table->iterations.resize(hosts_.size(), -1);
    comm_->CreateTable(config, end - start);
}


void Client::LocalAssign(const std::string& name, const void* data) {
    auto& table = tables_[name];
    table->cache->Assign(data);
}

void Client::ServerAssign(int server, const std::string& tablename, const void* data, int iteration) {
    auto& table = tables_[tablename];
    int start = server ? table->host_ends[server-1] : 0;
    int end = table->host_ends[server];
    {
        std::lock_guard<std::mutex> lock(table->mu);
        table->cache->Assign(data, start, end - start);
        if (table->iterations[server] < iteration) {
            table->iterations[server] = iteration;
        }
    }
    table->cv.notify_all();
}

void Client::Update(const std::string& name, const void* data) {
    auto& table = tables_[name];
    auto& ends = table->host_ends;
    int start = 0;
    for (size_t server = 0; server < hosts_.size(); ++server) {
        int end = ends[server];
        size_t size = (end - start) * table->element_size;
        size_t offset = start * table->element_size;
        int8_t* offset_data = (int8_t*)data + offset;

        comm_->Update(server, name, offset_data, size, iteration_);

        start = end;
    }
}

void Client::Clock() {
    iteration_++;    
}

void Client::Sync(const std::string& name) {
    auto& table = tables_[name];
    std::unique_lock<std::mutex> lock(table->mu);
    int min = *std::min_element(
            table->iterations.begin(), table->iterations.end());
    if (min < iteration_ - staleness_ - 1) {
        for (size_t server = 0; server < hosts_.size(); ++server) {
            comm_->Sync(server, name, iteration_);
        }

        table->cv.wait(lock, [this, &name, &table]{
            int min = *std::min_element(
                    table->iterations.begin(), table->iterations.end());
            return min >= iteration_ - staleness_ - 1;
        });
    }
}

void Client::ForceSync() {
    LOG(INFO) << "ForceSync";
    for (auto& pair: tables_) {
        auto& name = pair.first;
        auto& table = pair.second;
        auto& ends = table->host_ends;
        int start = 0;
        for (size_t server = 0; server < hosts_.size(); ++server) {
            int end = ends[server];
            auto data = table->cache->Serialize();
            const void *offset_data;
            size_t size;
            if (this_host_ == 0) {
                size = (end - start) * table->element_size;
                size_t offset = start * table->element_size;
                offset_data = static_cast<const int8_t*>(data) + offset;
            }
            comm_->ForceSync(server, name, offset_data, size);

            start = end;
        }
    }
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
