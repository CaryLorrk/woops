#ifndef WOOPS_LIB_H_
#define WOOPS_LIB_H_

#include <string>

#include "util/typedef.h"
#include "util/config/woops_config.h"
#include "util/config/table_config.h"
#include "util/comm/comm.h"
#include "util/placement/placement.h"
#include "consistency/consistency.h"
#include "client/client.h"
#include "server/server.h"

namespace woops
{
class Lib
{
public:
    static void Initialize(const WoopsConfig& config); 
    static void InitializeFromFile(const std::string& filename);

    static void CreateTable(const TableConfig& config);
    static void LocalAssign(Tableid id, const Storage& data);
    static void LocalUpdate(Tableid id, const Storage& data);
    static void Update(Tableid id, const Storage& data);
    static void Clock();
    static void Sync(Tableid id);
    static void Start();
    static std::string ToString();

    static woops::Comm& Comm() {
        Lib& lib = Get();
        return *lib.comm_.get();
    }

    static woops::Placement& Placement() {
        Lib& lib = Get();
        return *lib.placement_.get();
    }

    static woops::Client& Client() {
        Lib& lib = Get();
        return *lib.client_.get();
    }

    static woops::Server& Server() {
        Lib& lib = Get();
        return *lib.server_.get();
    }

    static woops::Consistency& Consistency() {
        Lib& lib = Get();
        return *lib.consistency_.get();
    }

    static std::vector<std::string> Hosts() {
        Lib& lib = Get();
        return lib.woops_config_.hosts;
    }

    static std::string Port() {
        Lib& lib = Get();
        return lib.woops_config_.port;
    }

    static Hostid NumHosts() {
        Lib& lib = Get();
        return static_cast<Hostid>(lib.woops_config_.hosts.size());
    }

    static Hostid ThisHost() {
        Lib& lib = Get();
        return lib.woops_config_.this_host;
    }

    static std::vector<woops::TableConfig>& TableConfigs() {
        Lib& lib = Get();
        return lib.table_configs_;
    }


private:
    WoopsConfig woops_config_;
    std::vector<woops::TableConfig> table_configs_;

    std::unique_ptr<woops::Consistency> consistency_;
    std::unique_ptr<woops::Placement> placement_;
    std::unique_ptr<woops::Client> client_;
    std::unique_ptr<woops::Server> server_;
    std::unique_ptr<woops::Comm> comm_;
    static Lib& Get() {
        static Lib lib;
        return lib;
    }
    
};
} /* woops */ 



#endif /* end of include guard: WOOPS_LIB_H_ */
