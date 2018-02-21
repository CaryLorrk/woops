#include "server.h"

#include <mutex>

#include "lib.h"

namespace woops
{

void Server::CreateTable(const TableConfig& config, size_t size) {
    std::lock_guard<std::mutex> lock(tables_mu_);
    auto pair = tables_.emplace(config.id, std::make_unique<ServerTable>());
    auto& table = pair.first->second;

    table->storage = config.server_storage_constructor(size);
    table->storage->Zerofy();
    table->size = size;
    table->element_size = config.element_size;

    table->iterations.resize(Lib::NumHosts(), -1);
}

void Server::Assign(int id, const void* data) {
    auto& table = tables_[id];
    std::lock_guard<std::mutex> lock(table->mu);
    table->storage->Assign(data);
}

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

//void Server::Update(int client, int id, const void* delta, size_t len, int iteration) {
void Server::Update(int client, int id, const Bytes& bytes, int iteration) {
    auto& table = tables_[id];
    std::lock_guard<std::mutex> lock(table->mu);
    //Bytes bytes((const char*)delta, len);
    //table->storage->Update(bytes.data());
    table->storage->Decoding(bytes);
    if (table->iterations[client] < iteration) {
        table->iterations[client] = iteration;
    }
    int min = *std::min_element(table->iterations.begin(), table->iterations.end());
    if (min >= iteration - Lib::Staleness()) {
        table->cv.notify_all();
    }
}

const void* Server::GetParameter(int id,
        int& iteration, size_t& size) {
    auto& table = tables_[id];
    auto& storage = table->storage;
    int min;
    std::unique_lock<std::mutex> lock(table->mu); 
    table->cv.wait(lock, [this, &table, iteration, &min]{
        min = *std::min_element(table->iterations.begin(), table->iterations.end());
        return min >= iteration - Lib::Staleness();
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
