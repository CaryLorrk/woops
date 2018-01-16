#include "uniform_split_placement.h"

namespace woops
{

void UniformSplitPlacement::Initialize(const WoopsConfig& config) {
    num_hosts_ = config.hosts.size();
}

void UniformSplitPlacement::Decision() {
    for(auto& config: configs_) {
        auto& partitions = table_to_partitions_[config.id];
        int div = config.size / num_hosts_;
        int mod = config.size % num_hosts_;
        int prev = 0;
        int idx = 0;
        for (size_t server = 0; server < num_hosts_; ++server) {
            idx += server < (num_hosts_ - mod) ? div : div+1;
            partitions[server] = Partition{prev, idx};
            prev = idx;
        }
    }
}

} /* woops */ 
