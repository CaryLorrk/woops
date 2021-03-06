#ifndef WOOPS_UTIL_COMM_COMM_H_
#define WOOPS_UTIL_COMM_COMM_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <grpc++/grpc++.h>

#include "comm_server.h"

namespace woops
{

class CommServer;
class Comm
{
public:
    void Initialize();
    void ClientPush(Hostid server, Tableid id, Iteration iteration, Bytes&& bytes);
    void ClientPull(Hostid server, Tableid id, Iteration iteration);
    void ServerPush(Hostid client, Tableid id, Iteration iteration, Bytes&& bytes);
    void ServerPull(Hostid client, Tableid id, Iteration iteration);
    void SyncStorage(Hostid host, Tableid id, Bytes&& data);
    void SyncPlacement();

    void Barrier();

    ~Comm();
private:
    static constexpr size_t MAX_MESSAGE_SIZE = 100*1024*1024;
    void build_rpc_server();
    std::unique_ptr<CommServer> service_;
    std::unique_ptr<grpc::Server> rpc_server_;
    std::thread rpc_server_thread_;
    void rpc_server_func();

    void create_stubs();
    std::vector<std::unique_ptr<rpc::Comm::Stub>> stubs_;

    void create_streams();
    std::unique_ptr<std::mutex[]> client_push_streams_mu_;
    std::vector<std::unique_ptr<grpc::ClientContext>> client_push_ctxs_;
    std::vector<std::unique_ptr<grpc::ClientReaderWriter<rpc::PushRequest, rpc::PushResponse>>> client_push_streams_;

    std::unique_ptr<std::mutex[]> client_pull_streams_mu_;
    std::vector<std::unique_ptr<grpc::ClientContext>> client_pull_ctxs_;
    std::vector<std::unique_ptr<grpc::ClientReaderWriter<rpc::PullRequest, rpc::PullResponse>>> client_pull_streams_;

    std::unique_ptr<std::mutex[]> server_push_streams_mu_;
    std::vector<std::unique_ptr<grpc::ClientContext>> server_push_ctxs_;
    std::vector<std::unique_ptr<grpc::ClientReaderWriter<rpc::PushRequest, rpc::PushResponse>>> server_push_streams_;

    std::unique_ptr<std::mutex[]> server_pull_streams_mu_;
    std::vector<std::unique_ptr<grpc::ClientContext>> server_pull_ctxs_;
    std::vector<std::unique_ptr<grpc::ClientReaderWriter<rpc::PullRequest, rpc::PullResponse>>> server_pull_streams_;

    // finish
    void finish_handler();
    bool is_finish = false;
    void finish();

    // Barrier
    void barrier_notified();
    std::mutex barrier_mu_;
    std::condition_variable barrier_cv_;
    int barrier_cnt_ = 0;


friend class CommServer;
};

    
} /* woops */ 



#endif /* end of include guard: WOOPS_UTIL_COMM_COMM_H_ */
