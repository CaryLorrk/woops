#ifndef WOOPS_UTIL_COMM_COMM_H_
#define WOOPS_UTIL_COMM_COMM_H_

#include <thread>

#include <grpc++/grpc++.h>
#include "server/ps_service_server.h"

#include "util/config/woops_config.h"

namespace woops
{

class Client;
class PsServiceServer;
class Comm
{
public:
    void CreateTable(const TableConfig& config, size_t size);
    void Update(int server, const std::string& tablename,
            const void* data, size_t size, int iteration);
    void Sync(int server, const std::string& tablename, int iteration);
    void ForceSync(int server, const std::string tablename, const void* data, size_t size);
    ~Comm();

    void Initialize(const WoopsConfig& config, Client *client);

private:
    /* config */
    int this_host_;
    int staleness_;
    std::string port_;
    std::vector<std::string> hosts_;

    Client *client_;

    std::unique_ptr<PsServiceServer> service_;
    std::unique_ptr<grpc::Server> server_;
    std::thread server_thread_;
    std::vector<std::unique_ptr<rpc::PsService::Stub>> stubs_;
    std::vector<std::thread> client_threads_;

    std::unique_ptr<std::mutex[]> push_streams_mu_;
    std::vector<std::unique_ptr<grpc::ClientContext>> push_ctxs_;
    std::vector<std::unique_ptr<grpc::ClientReaderWriter<rpc::UpdateRequest, rpc::UpdateResponse>>> push_streams_;

    std::vector<std::unique_ptr<grpc::ClientContext>> pull_ctxs_;
    std::unique_ptr<std::mutex[]> pull_streams_mu_;
    std::vector<std::unique_ptr<grpc::ClientReaderWriter<rpc::PullRequest, rpc::PullResponse>>> pull_streams_;

    void server_thread_func_();
    void client_thread_func_(int server);
};
    
} /* woops */ 



#endif /* end of include guard: WOOPS_UTIL_COMM_COMM_H_ */
