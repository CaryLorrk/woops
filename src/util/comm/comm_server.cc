#include "comm_server.h"
#include "lib.h"

#include <cstdlib>
#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>

#include "util/storage/dense_storage.h"
#include "util/logging.h"
#include "util/comm/comm.h"
#include "server/server.h"
#include "client/client.h"

namespace woops
{
grpc::Status CommServer::CheckAlive(grpc::ServerContext* ctx,
        const rpc::CheckAliveRequest* req,
        rpc::CheckAliveResponse* res){
    res->set_status(true);
    return grpc::Status::OK;
}

grpc::Status CommServer::Finish(grpc::ServerContext* ctx,
        const rpc::FinishRequest* req, rpc::FinishResponse* res) {
    std::abort();
}

grpc::Status CommServer::SyncPlacement(grpc::ServerContext* ctx,
        const rpc::SyncPlacementRequest* req, rpc::SyncPlacementResponse* res) {
    res->set_data(Lib::Placement()->Serialize());
    return grpc::Status::OK;
}

grpc::Status CommServer::BarrierNotify(grpc::ServerContext* ctx,
        const rpc::BarrierNotifyRequest* req,
        rpc::BarrierNotifyResponse* res){
    Lib::Comm()->barrier_notified_();
    return grpc::Status::OK;
}

grpc::Status CommServer::Assign(grpc::ServerContext* ctx,
        const rpc::AssignRequest* req, rpc::AssignResponse* res) {
    int host = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    int id = req->tableid();
    Lib::Client()->ServerAssign(host, id, req->parameter(), -1);
    return grpc::Status::OK;
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

grpc::Status CommServer::Update(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::UpdateResponse, rpc::UpdateRequest>* stream) {
    int client = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::UpdateRequest req;
    while (stream->Read(&req)) {
        Bytes bytes(req.delta());
        Lib::Server()->Update(client, req.tableid(), bytes, req.iteration());        
    }
    return grpc::Status::OK;
}

grpc::Status CommServer::Pull(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::PullResponse, rpc::PullRequest>* stream) {
    int client = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::PullRequest req;
    std::mutex stream_mu;
    while(stream->Read(&req)) {
        std::thread t([this, req, &stream_mu, &stream, client] {
            int id = req.tableid();
            int iteration = req.iteration();
            Bytes parameter = Lib::Server()->GetParameter(client, id, iteration);
            Lib::Comm()->Push(client, id, std::move(parameter), iteration);
        });
        t.detach();
        
    }
    return grpc::Status::OK;
}

grpc::Status CommServer::Push(grpc::ServerContext* ctx,
        grpc::ServerReaderWriter<rpc::PushResponse, rpc::PushRequest>* stream) {
    int server = std::stoi(ctx->client_metadata().find("from_host")->second.data());
    rpc::PushRequest req;
    while(stream->Read(&req)) {
        Bytes bytes(req.parameter());
        Lib::Client()->ServerUpdate(server, req.tableid(), bytes, req.iteration());
    }
    return grpc::Status::OK;
}
} /* woops */ 
