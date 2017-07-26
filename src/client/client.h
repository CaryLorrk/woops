#ifndef WOOPS_CLINET_CLIENT_H_
#define WOOPS_CLINET_CLIENT_H_

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include <grpc++/grpc++.h>
#include "server/ps_service_server.h"

#include "woops_config.h"
#include "table_config.h"
#include "table.h"

namespace woops
{

class Client final
{
public:
    static Client& GetInstance();
    void Initialize(const WoopsConfig& config);
    void CreateTable(const TableConfig& config);
    void LocalAssign(const std::string& name, const void* data);
    void Update(const std::string& name, const void* data);
    void Clock();
    void Sync(const std::string& name);
    void ForceSync();
    std::string ToString();

    ~Client();
private:
    /* config */
    int this_host_;
    int staleness_;
    std::string port_;
    std::vector<std::string> hosts_;

    std::map<std::string, std::unique_ptr<Table>> tables_;
    std::atomic<int> iteration_;

    /* rpc */
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

    Client(): iteration_(0){}
};
    
} /* woops */ 


#endif /* end of include guard: WOOPS_CLINET_CLIENT_H_ */
