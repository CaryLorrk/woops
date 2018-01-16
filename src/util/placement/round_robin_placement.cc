#include "round_robin_placement.h"

namespace woops
{

void RoundRobinPlacement::Initialize(const WoopsConfig& config) {
    num_hosts_ = config.hosts.size();
}

void RoundRobinPlacement::Decision() {
    int last = 0;
    for (auto& config: configs_) {
        table_to_partitions_[config.id][last] = Partition{0, config.size};
        last = (last + 1) % num_hosts_;
    }
}

    
} /* woops */ 
