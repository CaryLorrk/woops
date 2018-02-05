#ifndef WOOPS_SERVER_PS_SERVICE_SERVER_H_
#define WOOPS_SERVER_PS_SERVICE_SERVER_H_

#include <map>
#include <string>

#include "util/protobuf/ps_service.grpc.pb.h"
#include "util/storage/storage.h"

namespace woops
{
class PsServiceServer final: public rpc::PsService::Service
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

    grpc::Status ForceSync(grpc::ServerContext* ctx,
            const rpc::ForceSyncRequest* req, rpc::ForceSyncResponse* res) override;

    grpc::Status Update(grpc::ServerContext* ctx,
            grpc::ServerReaderWriter<rpc::UpdateResponse, rpc::UpdateRequest>* stream) override;

    grpc::Status Pull(grpc::ServerContext* ctx,
            grpc::ServerReaderWriter<rpc::PullResponse, rpc::PullRequest>* stream) override;

    grpc::Status Push(grpc::ServerContext* ctx,
            grpc::ServerReaderWriter<rpc::PushResponse, rpc::PushRequest>* stream) override;
};
} /* woops */ 

#endif /* end of include guard: WOOPS_SERVER_PS_SERVICE_SERVER_H_ */
