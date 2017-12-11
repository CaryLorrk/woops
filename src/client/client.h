#ifndef WOOPS_CLINET_CLIENT_H_
#define WOOPS_CLINET_CLIENT_H_

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <vector>


#include "util/config/woops_config.h"
#include "util/config/table_config.h"
#include "client_table.h"


namespace woops
{

class Comm;
class Client
{
public:
    void Initialize(const WoopsConfig& config, Comm* comm);
    void CreateTable(const TableConfig& config);
    void LocalAssign(const std::string& name, const void* data);
    void ServerAssign(int server, const std::string& tablename, const void* data, int iteration);
    void Update(const std::string& name, const void* data);
    void Clock();
    void Sync(const std::string& name);
    void ForceSync();
    std::string ToString();

    Client();
private:
    Comm* comm_;

    /* config */
    int this_host_;
    int staleness_;
    std::string port_;
    std::vector<std::string> hosts_;

    std::map<std::string, std::unique_ptr<ClientTable>> tables_;
    std::atomic<int> iteration_;
}; /* woops */ 
}


#endif /* end of include guard: WOOPS_CLINET_CLIENT_H_ */
