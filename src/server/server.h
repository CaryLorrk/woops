#ifndef WOOPS_SERVER_SERVER_H_
#define WOOPS_SERVER_SERVER_H_

#include <string>
#include <tuple>

#include "util/config/table_config.h"
#include "util/config/woops_config.h"
#include "server_table.h"

namespace woops
{
class Comm;
class Server
{
public:
    void ClientPushHandler(Hostid client, Tableid id, Iteration iteration, const Bytes& bytes);
    void CreateTable(const TableConfig& config, size_t size);
    std::tuple<Iteration, Bytes> GetData(Hostid client, Tableid id, Iteration iteration);
    std::string ToString();

    ServerTable& GetTable(Tableid id);

private:
    std::map<Tableid, std::unique_ptr<ServerTable>> tables_;
    std::mutex tables_mu_;
}; 
} /* woops */ 


#endif /* end of include guard: WOOPS_SERVER_SERVER_H_ */
