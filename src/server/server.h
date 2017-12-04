#ifndef WOOPS_SERVER_SERVER_H_
#define WOOPS_SERVER_SERVER_H_

#include <string>

#include "util/config/table_config.h"
#include "util/config/woops_config.h"
#include "server_table.h"

namespace woops
{
class Comm;
class PsServiceServer;
class Server
{
public:
    void Assign(int client, const std::string& tablename, const void* data);
    void LocalAssign(const std::string& name, const void* data);
    void LocalUpdate(const std::string& name, const void* delta, int iteration);
    void CreateTable(const TableConfig& config, size_t size);
    std::string ToString();

    void Initialize(const WoopsConfig& config, Comm *comm);

private:
    Comm *comm_;

    int this_host_;
    size_t num_hosts_;
    int staleness_;
    std::map<std::string, std::unique_ptr<ServerTable>> tables_;
    std::mutex tables_mu_;
    std::condition_variable tables_cv_;

    std::unique_ptr<ServerTable>& GetTable(const std::string& name); 
friend class PsServiceServer;
}; 
} /* woops */ 


#endif /* end of include guard: WOOPS_SERVER_SERVER_H_ */
