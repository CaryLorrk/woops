#include "client.h"

#include <algorithm>

#include "common/protobuf/ps_service.grpc.pb.h"
#include "common/logging.h"

namespace woops
{ 

Client::~Client() {
    LOG(INFO) << "Clean up.";
    for (auto& s: streams_) {
        s->WritesDone();
    }
    server_->Shutdown();
    for (auto& s: streams_) {
        auto status = s->Finish();
        if (!status.ok()) {
            LOG(FATAL) << "Failed to finish stream. Error code: " << status.error_code();
        }
    }
    server_thread_.join();
    for (auto& t: client_threads_) {
        t.join();
    }
}

Client& Client::GetInstance() {
    static Client singleton;
    return singleton;
}

void Client::server_thread_func_() {
    server_->Wait();
}

void Client::client_thread_func_(int server) {
    rpc::UpdateResponse res;
    while (streams_[server]->Read(&res)) {
        auto& table = tables_[res.name()];
        int start = server ? table->host_ends[server-1] : 0;
        int end = table->host_ends[server];
        std::unique_lock<std::mutex> lock(table->mu);
        table->cache->Assign(res.param().data(), start, end - start);
        table->iterations[server] = res.iteration();
        table->cv.notify_all();
    }
}

using namespace std::chrono_literals;
void Client::Initialize(const WoopsConfig& config) {
    this_host_ = config.this_host;
    staleness_ = config.staleness;
    port_ = config.port;
    hosts_ = config.hosts;

    /* server */
    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:"+port_, grpc::InsecureServerCredentials());
    service_ = std::make_unique<PsServiceServer>(hosts_.size(), staleness_);
    builder.RegisterService(service_.get());
    server_ = builder.BuildAndStart();
    server_thread_ = std::thread(&Client::server_thread_func_, this);
    std::this_thread::sleep_for(100ms);

    /* stubs */
    LOG(INFO) << "Check servers:";
    for (auto& host: hosts_) {
        while(1) {
            auto stub = rpc::PsService::NewStub(grpc::CreateChannel(
                            host+":"+port_, grpc::InsecureChannelCredentials()));
            grpc::ClientContext ctx;
            rpc::CheckAliveRequest req;
            rpc::CheckAliveResponse res;
            stub->CheckAlive(&ctx, req, &res);
            if (res.status()) {
                LOG(INFO) << host << " is up.";
                stubs_.push_back(std::move(stub));
                break;
            } else {
                LOG(WARNING) << "Failed to connect to " << host <<".";
                std::this_thread::sleep_for(1s);
            }
        }
    }

    /* Client */
    for (size_t server = 0; server < hosts_.size(); server++) {
        auto ctx = std::make_unique<grpc::ClientContext>();
        ctx->AddMetadata("client", std::to_string(this_host_));
        streams_.push_back(stubs_[server]->Update(ctx.get()));
        ctxs_.push_back(std::move(ctx));
        client_threads_.emplace_back(&Client::client_thread_func_, this, server);
    }
    std::this_thread::sleep_for(100ms);
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
    service_->CreateTable(config, end - start);
}


void Client::LocalAssign(const std::string& name, const void* data) {
    auto& table = tables_[name];
    table->cache->Assign(data);
}

void Client::Update(const std::string& name, const void* data) {
    auto& table = tables_[name];
    auto& ends = table->host_ends;
    int start = 0;
    for (size_t server = 0; server < hosts_.size(); ++server) {
        int end = ends[server];

        rpc::UpdateRequest req;
        req.set_name(name);
        size_t offset = start * table->element_size;
        size_t size = (end - start) * table->element_size;
        req.set_delta((int8_t*)data + offset, size);
        streams_[server]->Write(req);

        start = end;
    }
}

void Client::Clock() {
    iteration_++;    
}

void Client::Sync(const std::string& name) {
    auto& table = tables_[name];
    std::unique_lock<std::mutex> lock(table->mu);
    table->cv.wait(lock, [this, &name, &table]{
        int min = *std::min_element(
                table->iterations.begin(), table->iterations.end());
        return min >= iteration_ - staleness_ - 1;
    });
}

void Client::ForceSync() {
    for (auto& pair: tables_) {
        auto& name = pair.first;
        auto& table = pair.second;
        auto& ends = table->host_ends;
        int start = 0;
        for (size_t server = 0; server < hosts_.size(); ++server) {

            int end = ends[server];
            auto data = table->cache->Serialize();

            rpc::AssignRequest req;
            req.set_client(this_host_);
            req.set_name(name);
            if (this_host_ == 0) {
                size_t offset = start * table->element_size;
                size_t size = (end - start) * table->element_size;
                req.set_param((int8_t*)data + offset, size);
            }

            grpc::ClientContext ctx;
            rpc::AssignResponse res;
            stubs_[server]->Assign(&ctx, req, &res);
            if (this_host_ != 0) {
                table->cache->Assign(res.param().data(), start, end - start);
            }
            start = end;
        }
    }
}

} /* woops */ 
