#ifndef WOOPS_LIB_H_
#define WOOPS_LIB_H_

#include <string>

#include "util/config/woops_config.h"
#include "util/comm/comm.h"
#include "client/client.h"
#include "server/server.h"
#include "server/ps_service_server.h"

namespace woops
{
class Lib
{
public:
    static void Initialize(const WoopsConfig& config); 
    static void InitializeFromFile(const std::string& filename);

    static void CreateTable(const TableConfig& config);
    static void LocalAssign(const std::string& tablename, const void* data);
    static void Update(const std::string& tablename, const void* data);
    static void Clock();
    static void Sync(const std::string& tablename);
    static void ForceSync();
    static std::string ToString();

private:
    WoopsConfig config_;
    Client client_;
    Server server_;
    Comm comm_;
    static Lib& Get();
    
};
} /* woops */ 



#endif /* end of include guard: WOOPS_LIB_H_ */