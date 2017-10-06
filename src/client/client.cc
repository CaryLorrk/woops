#include "client.h"

#include <iostream>
#include <algorithm>
#include <functional>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include "common/context.h"
#include "common/logging.h"

using namespace std::placeholders;

namespace woops
{ 

Client::~Client() {
    LOG(INFO) << "Clean up.";
}

void Client::Initialize() {
    Context::GetComm().RegisterMessageHandler(
            rpc::Message_Command_FORCE_SYNC,
            std::bind(&Client::HandleForceSync, this, _1, _2));
    Context::GetComm().RegisterMessageHandler(
            rpc::Message_Command_PULL,
            std::bind(&Client::HandlePull, this, _1, _2));
}

void Client::CreateTable(const TableConfig& config) {
    auto pair = tables_.emplace(config.name, std::make_unique<Table>());
    auto& table = pair.first->second;
    table->size = config.size;
    table->element_size = config.element_size;
    table->cache = config.cache_constructor(table->size);


    /* split table */
    int div = table->size / Context::NumHosts();
    int mod = table->size % Context::NumHosts();
    int idx = 0;
    auto& ends = table->host_ends;
    for (size_t i = 0; i < Context::NumHosts(); ++i) {
        idx += i < (Context::NumHosts() - mod) ? div : div+1;
        ends.push_back(idx);
    }
    unsigned this_host = Context::ThisHost();
    int start = this_host == 0 ? 0 : ends[this_host-1];
    int end = ends[this_host];


    table->iterations.resize(Context::NumHosts(), 0);
    Context::GetServer().CreateTable(config, end - start);
}


void Client::LocalAssign(const std::string& name, const void* data) {
    auto& table = tables_[name];
    table->cache->Assign(data);
}

void Client::Update(const std::string& name, const void* data) {
    //LOG(INFO) << "In Update table: " << name << ", iteration: " << iteration_;
    auto& table = tables_[name];
    auto& ends = table->host_ends;
    int start = 0;
    std::lock_guard<std::mutex> lock(table->mu);
    for (size_t server = 0; server < Context::NumHosts(); ++server) {
        int end = ends[server];
        size_t size = (end - start) * table->element_size;
        size_t offset = start * table->element_size;
        int8_t* offset_data = (int8_t*)data + offset;

        if (server == Context::ThisHost()) {
            Context::GetServer().Update(name, offset_data, iteration_);
        } else {
            auto update = std::make_unique<rpc::Update>();
            update->set_tablename(name);
            update->set_iteration(iteration_);
            update->set_delta(offset_data, size);

            rpc::Message msg;            
            msg.set_cmd(rpc::Message_Command_UPDATE);
            msg.set_allocated_update(update.release());
        
            Context::GetComm().SendMessage(server, msg);
        }
        start = end;
    }
}

void Client::Clock() {
    iteration_++;    
}

void Client::Sync(const std::string& tablename) {
    auto& table = tables_[tablename];
    std::unique_lock<std::mutex> lock(table->mu);
    unsigned min = *std::min_element(
            table->iterations.begin(), table->iterations.end());
    if (min < iteration_ - Context::Staleness() - 1) {
        auto pull_req = std::make_unique<rpc::PullReq>();
        pull_req->set_tablename(tablename);
        pull_req->set_iteration(iteration_ - 1);

        rpc::Message msg;
        msg.set_cmd(rpc::Message_Command_PULL_REQ);
        msg.set_allocated_pull_req(pull_req.release());

        for (unsigned server = 0; server < Context::NumHosts(); ++server) {
            if (server == Context::ThisHost()) {
                int start = server ? table->host_ends[server - 1] : 0;
                int end = table->host_ends[server];
                int size = (end - start) * table->element_size;
                std::vector<uint8_t> parameter(size);
                unsigned iter;
                Context::GetServer().Read(tablename, iteration_ - 1, parameter.data(), iter);
                table->cache->Assign(parameter.data(), start, end - start);
                if (table->iterations[server] < iter) {
                    table->iterations[server] = iter;
                }
                table->cv.notify_all();
            } else {
                Context::GetComm().SendMessage(server, msg);
            }
        }

        table->cv.wait(lock, [this, &tablename, &table]{
            unsigned min = *std::min_element(
                    table->iterations.begin(), table->iterations.end());
            return min >= iteration_ - Context::Staleness() - 1;
        });
    }

}

void Client::HandleForceSync(unsigned host, const rpc::Message& msg){
    if(host != 0) {
        LOG(FATAL) << "ForceSync from host " << host;
    }
    auto& cmd_msg = msg.force_sync();
    //LOG(INFO) << "HandleForceSync from host " << host; 
    for (int i = 0; i  < cmd_msg.tablenames_size(); ++i) {
        Context::GetServer().Assign(cmd_msg.tablenames(i), cmd_msg.parameters(i).data());
    }
    std::lock_guard<std::mutex> lock(sync_mu_);
    sync_ = true;
    sync_cv.notify_all();
}

void Client::HandlePull(unsigned host, const rpc::Message& msg) {
    auto& cmd_msg = msg.pull();
    auto& tablename = cmd_msg.tablename();
    unsigned iteration = cmd_msg.iteration();
    //LOG(INFO) << "HandlePull from host " << host << " table " << tablename << " iteration " << iteration;
    auto& table = tables_[tablename];
    int start = host ? table->host_ends[host-1] : 0;
    int end = table->host_ends[host];
    std::lock_guard<std::mutex> lock(table->mu);
    table->cache->Assign(cmd_msg.parameter().data(), start, end - start);
    if (table->iterations[host] < iteration) {
        table->iterations[host] = iteration;
    }
    table->cv.notify_all();
}

void Client::ForceSync() {
    LOG(INFO) << "ForceSync";
    if (Context::ThisHost() == 0) {
        for (unsigned host = 0; host < Context::NumHosts(); ++host) {
            auto force_sync = std::make_unique<rpc::ForceSync>();
            for (auto& kv: tables_) {
                auto& tablename = kv.first;
                auto& table = kv.second;
                auto& ends = table->host_ends;
                int start = host > 0 ? ends[host - 1] : 0;
                int end = ends[host];
                auto data = table->cache->Serialize();
                size_t offset = start * table->element_size;
                size_t size = (end - start) * table->element_size;

                if (host == 0) {
                    Context::GetServer().Assign(tablename, (int8_t*)data + offset);
                } else {
                    force_sync->add_tablenames(tablename);
                    force_sync->add_parameters((int8_t*)data + offset, size);
                }
            }
            if (host != 0) {
                rpc::Message msg;
                msg.set_cmd(rpc::Message_Command_FORCE_SYNC);
                msg.set_allocated_force_sync(force_sync.release());
                Context::GetComm().SendMessage(host, msg);
            }
        }
    } else {
        std::unique_lock<std::mutex> lock(sync_mu_);
        if (sync_ == false) {
            sync_cv.wait(lock);
        }
    }
    sync_ = false;
}

std::string Client::ToString() {
    std::stringstream ss;
    ss << "Client: " << std::endl;    
    for (auto& pair: tables_) {
        ss << pair.first << ": " << pair.second->cache->ToString() << std::endl;
    }
    ss << "Server: " << std::endl;
    return ss.str();
}

} /* woops */ 
