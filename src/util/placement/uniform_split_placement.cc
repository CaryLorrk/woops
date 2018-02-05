#include "uniform_split_placement.h"

#include "lib.h"

namespace woops
{

void UniformSplitPlacement::Decision() {
    for(auto& config: Lib::TableConfigs()) {
        auto& partitions = table_to_partitions_[config.id];
        int div = config.size / Lib::NumHosts();
        int mod = config.size % Lib::NumHosts();
        int prev = 0;
        int idx = 0;
        for (int server = 0; server < Lib::NumHosts(); ++server) {
            idx += server < (Lib::NumHosts() - mod) ? div : div+1;
            partitions[server] = Partition{prev, idx};
            prev = idx;
        }
    }
}

} /* woops */ 
