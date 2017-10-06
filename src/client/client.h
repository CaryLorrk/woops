#ifndef WOOPS_CLINET_CLIENT_H_
#define WOOPS_CLINET_CLIENT_H_

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "common/protobuf/rpc_message.pb.h"
#include "table_config.h"
#include "table.h"

namespace woops
{

class Client 
{
public:
    Client(): iteration_(1), sync_(false){}
    void Initialize();
    void CreateTable(const TableConfig& config);
    void LocalAssign(const std::string& name, const void* data);
    void Update(const std::string& name, const void* data);
    void Clock();
    void Sync(const std::string& name);
    void ForceSync();
    std::string ToString();

    void HandleForceSync(unsigned host, const rpc::Message& msg);
    void HandlePull(unsigned host, const rpc::Message& msg);

    ~Client();
private:
    /* config */

    std::map<std::string, std::unique_ptr<Table>> tables_;
    std::atomic<int> iteration_;
    
    bool sync_;
    std::mutex sync_mu_;
    std::condition_variable sync_cv;


};
    
} /* woops */ 


#endif /* end of include guard: WOOPS_CLINET_CLIENT_H_ */
