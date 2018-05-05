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

class Client
{
public:
    void CreateTable(const TableConfig& config);
    void LocalAssign(Tableid id, const Storage& data);
    void LocalUpdate(Tableid id, const Storage& data);
    void Update(Tableid id, const Storage& data);
    void Clock();
    void Sync(Tableid id);
    void Start();

    void ServerSyncStorage(Tableid id,const Bytes& bytes);
    void ServerUpdate(Hostid server, Tableid id,const Bytes& bytes, int iteration);
    std::string ToString();

    Client();
private:
    std::map<int, std::unique_ptr<ClientTable>> tables_;
    std::atomic<int> iteration_;

    void sync_placement();
    void sync_server();
    void sync_client();

}; /* woops */ 
}


#endif /* end of include guard: WOOPS_CLINET_CLIENT_H_ */
