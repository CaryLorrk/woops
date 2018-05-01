#ifndef WOOPS_SERVER_SERVER_H_
#define WOOPS_SERVER_SERVER_H_

#include <string>

#include "util/config/table_config.h"
#include "util/config/woops_config.h"
#include "server_table.h"

namespace woops
{
class Comm;
class Server
{
public:
    //void Assign(Tableid tableid, const Bytes& bytes);
    void Update(Hostid client, Tableid tableid, const Bytes& bytes, Iteration iteration);
    void CreateTable(const TableConfig& config, size_t size);
    Bytes GetParameter(Hostid client, Tableid tableid, Iteration& iteration);
    std::string ToString();

private:
    std::map<Hostid, std::unique_ptr<ServerTable>> tables_;
    std::mutex tables_mu_;
}; 
} /* woops */ 


#endif /* end of include guard: WOOPS_SERVER_SERVER_H_ */
