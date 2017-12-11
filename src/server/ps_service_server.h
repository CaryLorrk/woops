#ifndef WOOPS_SERVER_PS_SERVICE_SERVER_H_
#define WOOPS_SERVER_PS_SERVICE_SERVER_H_

#include <map>
#include <string>

#include "util/protobuf/ps_service.grpc.pb.h"
#include "util/storage/storage.h"

namespace woops
{
class Comm;
class Client;
class Server;
class PsServiceServer final: public rpc::PsService::Service
{
public:
    PsServiceServer(Comm* comm, Client* client, Server* server);

    grpc::Status CheckAlive(grpc::ServerContext* ctx,
            const rpc::CheckAliveRequest* req, rpc::CheckAliveResponse* res) override;

    grpc::Status BarrierNotify(grpc::ServerContext* ctx,
            const rpc::BarrierNotifyRequest* req, rpc::BarrierNotifyResponse* res) override;

    grpc::Status ForceSync(grpc::ServerContext* ctx,
            const rpc::ForceSyncRequest* req, rpc::ForceSyncResponse* res) override;

    grpc::Status Update(grpc::ServerContext* ctx,
            grpc::ServerReaderWriter<rpc::UpdateResponse, rpc::UpdateRequest>* stream) override;

    grpc::Status Pull(grpc::ServerContext* ctx,
            grpc::ServerReaderWriter<rpc::PullResponse, rpc::PullRequest>* stream) override;

private:
    Comm* comm_;
    Client* client_;
    Server* server_;
};
} /* woops */ 

#endif /* end of include guard: WOOPS_SERVER_PS_SERVICE_SERVER_H_ */
