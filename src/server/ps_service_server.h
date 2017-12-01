#ifndef WOOPS_SERVER_PS_SERVICE_SERVER_H_
#define WOOPS_SERVER_PS_SERVICE_SERVER_H_

#include <map>
#include <string>

#include "util/protobuf/ps_service.grpc.pb.h"
#include "util/storage/storage.h"
#include "client/table.h"
#include "server_table.h"

namespace woops
{
class PsServiceServer final: public rpc::PsService::Service
{
public:
    PsServiceServer(int this_host, size_t num_hosts, int staleness);
    grpc::Status CheckAlive(grpc::ServerContext* ctx,
            const rpc::CheckAliveRequest* req, rpc::CheckAliveResponse* res) override;
    grpc::Status Assign(grpc::ServerContext* ctx,
            const rpc::AssignRequest* req, rpc::AssignResponse* res) override;
    grpc::Status Update(grpc::ServerContext* ctx,
            grpc::ServerReaderWriter<rpc::UpdateResponse, rpc::UpdateRequest>* stream) override;
    grpc::Status Pull(grpc::ServerContext* ctx,
            grpc::ServerReaderWriter<rpc::PullResponse, rpc::PullRequest>* stream) override;
    void LocalAssign(const std::string& name, const void* data);
    void LocalUpdate(const std::string& name, const void* delta, int iteration);
    void CreateTable(const TableConfig& config, size_t size);
    std::string ToString();

private:
    int this_host_;
    size_t num_hosts_;
    int staleness_;
    std::map<std::string, std::unique_ptr<ServerTable>> tables_;
    std::mutex tables_mu_;
    std::condition_variable tables_cv_;

    std::unique_ptr<ServerTable>& GetTable(const std::string& name); 
};
} /* woops */ 

#endif /* end of include guard: WOOPS_SERVER_PS_SERVICE_SERVER_H_ */
