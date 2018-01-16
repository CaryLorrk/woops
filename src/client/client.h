#ifndef WOOPS_CLINET_CLIENT_H_
#define WOOPS_CLINET_CLIENT_H_

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <vector>


#include "util/typedef.h"
#include "util/config/woops_config.h"
#include "util/config/table_config.h"
#include "client_table.h"


namespace woops
{

class Placement;
class Comm;
class Client
{
public:
    void Initialize(const WoopsConfig& config, Comm* comm, Placement* placement);
    void CreateTable(const TableConfig& config);
    void LocalAssign(Tableid id, const void* data);
    void ServerAssign(Hostid server, Tableid id, const void* data, int iteration);
    void Update(Tableid id, Storage& data);
    void Clock();
    void Sync(Tableid id);
    void ForceSync();
    std::string ToString();

    Client();
private:
    Comm* comm_;
    Placement* placement_;

    /* config */
    int this_host_;
    int staleness_;
    std::string port_;
    std::vector<std::string> hosts_;

    std::map<int, std::unique_ptr<ClientTable>> tables_;
    std::atomic<int> iteration_;

}; /* woops */ 
}


#endif /* end of include guard: WOOPS_CLINET_CLIENT_H_ */
