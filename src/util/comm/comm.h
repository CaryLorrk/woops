#ifndef WOOPS_UTIL_COMM_COMM_H_
#define WOOPS_UTIL_COMM_COMM_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <grpc++/grpc++.h>

#include "server/ps_service_server.h"
#include "util/config/woops_config.h"

namespace woops
{

class Client;
class Server;
class PsServiceServer;
class WoopsConfig;
class TableConfig;
class Comm
{
public:
    Comm();

    void Initialize(const WoopsConfig& config, Client *client, Server *server);
    void CreateTable(const TableConfig& config, size_t size);
    void Update(int server, const std::string& tablename,
            const void* data, size_t size, int iteration);
    void Pull(int server, const std::string& tablename, int iteration);
    void Push(int client, const std::string& tablename, const void* data, size_t size, int iteration);
    void ForceSync(int host, const std::string tablename, const void* data, size_t size);

    void Barrier();

    ~Comm();
private:
    /* config */
    int this_host_;
    int staleness_;
    std::string port_;
    std::vector<std::string> hosts_;

    Client *client_;
    Server *server_;

    std::unique_ptr<PsServiceServer> service_;
    std::unique_ptr<grpc::Server> rpc_server_;
    std::thread server_thread_;
    std::vector<std::unique_ptr<rpc::PsService::Stub>> stubs_;

    std::unique_ptr<std::mutex[]> update_streams_mu_;
    std::vector<std::unique_ptr<grpc::ClientContext>> update_ctxs_;
    std::vector<std::unique_ptr<grpc::ClientReaderWriter<rpc::UpdateRequest, rpc::UpdateResponse>>> update_streams_;

    std::vector<std::unique_ptr<grpc::ClientContext>> pull_ctxs_;
    std::unique_ptr<std::mutex[]> pull_streams_mu_;
    std::vector<std::unique_ptr<grpc::ClientReaderWriter<rpc::PullRequest, rpc::PullResponse>>> pull_streams_;

    std::vector<std::unique_ptr<grpc::ClientContext>> push_ctxs_;
    std::unique_ptr<std::mutex[]> push_streams_mu_;
    std::vector<std::unique_ptr<grpc::ClientReaderWriter<rpc::PushRequest, rpc::PushResponse>>> push_streams_;

    void server_thread_func_();

    // Barrier
    void barrier_notified_();
    std::mutex barrier_mu_;
    std::condition_variable barrier_cv_;
    int barrier_cnt_;

friend class PsServiceServer;
};

    
} /* woops */ 



#endif /* end of include guard: WOOPS_UTIL_COMM_COMM_H_ */
