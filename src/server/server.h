#ifndef WOOPS_SERVER_PS_SERVER_H_
#define WOOPS_SERVER_PS_SERVER_H_

#include <map>
#include <string>

#include "common/protobuf/rpc_message.pb.h"
#include "client/table.h"
#include "server_table.h"

namespace woops
{
class Server
{
public:
    void Initialize();
    void Assign(const std::string& tablename, const void* parameter);
    void Update(const std::string& tablename, const void* delta, unsigned iteration);
    void Read(const std::string& tablename, unsigned in_iter, void* parameter, unsigned& out_iter);
    void CreateTable(const TableConfig& config, size_t size);
    void HandlePullReq(unsigned host, rpc::Message msg);
    void HandleUpdate(unsigned host, rpc::Message msg);

private:
    std::map<std::string, std::unique_ptr<ServerTable>> tables_;
    std::mutex tables_mu_;
    std::condition_variable tables_cv_;


    std::unique_ptr<ServerTable>& GetTable(const std::string& name); 
};
} /* woops */ 



#endif /* end of include guard: WOOPS_SERVER_PS_SERVER_H_ */
