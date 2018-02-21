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
    void Update(int client, int id, const void* delta, size_t len, int iteration);
    void Update(int client, int id, const Bytes& bytes, int iteration);
    void CreateTable(const TableConfig& config, size_t size);
    const void* GetParameter(int id, int& iteration, size_t& size);
    std::string ToString();

private:
    std::map<int, std::unique_ptr<ServerTable>> tables_;
    std::mutex tables_mu_;
}; 
} /* woops */ 


#endif /* end of include guard: WOOPS_SERVER_SERVER_H_ */
