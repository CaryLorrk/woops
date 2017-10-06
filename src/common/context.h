#ifndef WOOPS_COMMON_GLOBAL_CONTEXT_H_
#define WOOPS_COMMON_GLOBAL_CONTEXT_H_

#include <string>
#include <vector>

#include "client/client.h"
#include "server/server.h"

#include "common/protobuf/woops_config.pb.h"
#include "comm.h"

namespace woops
{

class Context 
{
public:
    static void Initialize();
    static void Initialize(const WoopsConfigProto& configproto);

    static Comm& GetComm();
    static Client& GetClient();
    static Server& GetServer();

    static unsigned ThisHost();
    static std::string Hostname(unsigned host);
    static size_t NumHosts();
    static std::string Port();
    static unsigned Staleness();

private:
    unsigned this_host_;
    std::vector<std::string> hostnames_;
    std::string port_;
    unsigned staleness_;

    Comm comm_;
    Client client_;
    Server server_;
    
    void proto_init_hosts(const WoopsConfigProto& configproto);
    static Context& Get();
};
    
} /* woops */ 


#endif /* end of include guard: WOOPS_COMMON_GLOBAL_CONTEXT_H_ */
