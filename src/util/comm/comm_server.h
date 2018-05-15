#ifndef WOOPS_SERVER_PS_SERVICE_SERVER_H_
#define WOOPS_SERVER_PS_SERVICE_SERVER_H_

#include <map>
#include <string>

#include "util/protobuf/comm.grpc.pb.h"
#include "util/storage/storage.h"

namespace woops
{
class CommServer final: public rpc::Comm::Service
{
public:
    grpc::Status CheckAlive(grpc::ServerContext* ctx,
            const rpc::CheckAliveRequest* req, rpc::CheckAliveResponse* res) override;

    grpc::Status Finish(grpc::ServerContext* ctx,
            const rpc::FinishRequest* req, rpc::FinishResponse* res) override;

    grpc::Status SyncPlacement(grpc::ServerContext* ctx,
            const rpc::SyncPlacementRequest* req, rpc::SyncPlacementResponse* res) override;

    grpc::Status BarrierNotify(grpc::ServerContext* ctx,
            const rpc::BarrierNotifyRequest* req, rpc::BarrierNotifyResponse* res) override;

    grpc::Status SyncStorage(grpc::ServerContext* ctx,
            const rpc::SyncStorageRequest* req, rpc::SyncStorageResponse* res) override;

    grpc::Status ClientPush(grpc::ServerContext* ctx,
            grpc::ServerReaderWriter<rpc::PushResponse, rpc::PushRequest>* stream) override;

    grpc::Status ClientPull(grpc::ServerContext* ctx,
            grpc::ServerReaderWriter<rpc::PullResponse, rpc::PullRequest>* stream) override;

    grpc::Status ServerPush(grpc::ServerContext* ctx,
            grpc::ServerReaderWriter<rpc::PushResponse, rpc::PushRequest>* stream) override;

    grpc::Status ServerPull(grpc::ServerContext* ctx,
            grpc::ServerReaderWriter<rpc::PullResponse, rpc::PullRequest>* stream) override;
};
} /* woops */ 

#endif /* end of include guard: WOOPS_SERVER_PS_SERVICE_SERVER_H_ */
