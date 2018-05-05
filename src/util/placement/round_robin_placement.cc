#include "round_robin_placement.h"

#include "lib.h"

namespace woops
{
void RoundRobinPlacement::Decision() {
    int last = 0;
    for (auto&& config: Lib::TableConfigs()) {
        table_to_partitions_[config.id][last] = Partition{0, static_cast<ParamIndex>(config.size)};
        last = (last + 1) % Lib::NumHosts();
    }
}

    
} /* woops */ 
