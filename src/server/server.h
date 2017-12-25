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
    void Assign(int id, const void* data);
    void Update(int client, int id, const void* delta, int iteration);
    void CreateTable(const TableConfig& config, size_t size);
    const void* GetParameter(int id, int& iteration, size_t& size);
    std::string ToString();

    void Initialize(const WoopsConfig& config, Comm* comm);

private:
    Comm* comm_;

    int this_host_;
    size_t num_hosts_;
    int staleness_;
    std::map<int, std::unique_ptr<ServerTable>> tables_;
    std::mutex tables_mu_;
}; 
} /* woops */ 


#endif /* end of include guard: WOOPS_SERVER_SERVER_H_ */
