#ifndef WOOPS_UTIL_CONFIG_WOOPS_CONFIG_H_
#define WOOPS_UTIL_CONFIG_WOOPS_CONFIG_H_

#include <algorithm>
#include <vector>
#include <string>
#include <iostream>

#include "util/protobuf/woops_config.pb.h"
#include "util/logging.h"

namespace woops
{
struct WoopsConfig {
    int this_host;
    int staleness;
    std::string port;
    std::vector<std::string> hosts;

    WoopsConfig(const WoopsConfigProto& proto):
        this_host(-1),
        staleness(proto.staleness()),
        port(proto.port())
    {
        if (port.empty()) port = "50052";

        for (int i = 0; i < proto.hosts_size(); ++i) {
            auto& host = proto.hosts()[i];
            hosts.push_back(host);
            if (host == proto.this_host()) {
                this_host = i;
            }
        }
        if (proto.hosts_size() == 0 || this_host == -1) {
            LOG(FATAL) << "Configfile need hosts and this_host";
        }
    }

    WoopsConfig():
        this_host(0),
        staleness(0),
        port("50052"){}
}; 
    
} /* woops */ 


#endif /* end of include guard: WOOPS_UTIL_CONFIG_WOOPS_CONFIG_H_ */
