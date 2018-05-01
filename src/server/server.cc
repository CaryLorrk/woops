#include "server.h"

#include <mutex>

#include "lib.h"

namespace woops
{

void Server::CreateTable(const TableConfig& config, size_t size) {
    std::lock_guard<std::mutex> lock(tables_mu_);
    auto pair = tables_.emplace(config.id, std::make_unique<ServerTable>());
    auto& table = pair.first->second;

    table->storages.resize(Lib::NumHosts());
    for (int i = 0; i < Lib::NumHosts(); ++i) {
        table->storages[i] = config.server_storage_constructor();
        table->storages[i]->Zerofy();
    }

    table->size = size;
    table->element_size = config.element_size;

    table->iterations.resize(Lib::NumHosts(), -1);
}

//void Server::Assign(Tableid tableid, const Bytes& bytes) {
    //auto& table = tables_[tableid];
    //std::lock_guard<std::mutex> lock(table->mu);
    //table->storages[id]->Assign(bytes);
//}

static std::string string_to_hex(const std::string& input)
{
    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

static std::string chars_to_hex(const char* input, size_t len)
{
    static const char* const lut = "0123456789ABCDEF";

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

void Server::Update(Hostid client, Tableid tableid, const Bytes& bytes, Iteration iteration) {
    auto& table = tables_[tableid];
    std::lock_guard<std::mutex> lock(table->mu);
    for (int i = 0; i < Lib::NumHosts(); ++i) {
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

Bytes Server::GetParameter(Hostid client, Tableid tableid, Iteration& iteration) {
    auto& table = tables_[tableid];
    auto& storage = table->storages[client];
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
    for(auto& i: tables_) {
        //ss << i.first << ": " << i.second->storage->ToString() << std::endl; 
    }
    return ss.str();
}


 
} /* woops */ 
