#ifndef WOOPS_UTIL_COMM_COMM_H_
#define WOOPS_UTIL_COMM_COMM_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <grpc++/grpc++.h>

#include "comm_server.h"
#include "util/config/woops_config.h"

namespace woops
{

class CommServer;
class WoopsConfig;
class TableConfig;
class Comm
{
public:
    Comm();

    void Initialize();
    void Update(Hostid server, Tableid id, std::string& data, Iteration iteration);
    void Pull(Hostid server, Tableid id, Iteration iteration);
    void Push(Hostid client, Tableid id, Bytes bytes, Iteration iteration);
    void Assign(Hostid host, Tableid id, Bytes data);
    void SyncPlacement();

    void Barrier();

    ~Comm();
private:
    std::unique_ptr<CommServer> service_;
    std::unique_ptr<grpc::Server> rpc_server_;
    std::thread server_thread_;
    std::vector<std::unique_ptr<rpc::Comm::Stub>> stubs_;

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

    void finish_();

friend class CommServer;
};

    
} /* woops */ 



#endif /* end of include guard: WOOPS_UTIL_COMM_COMM_H_ */
