#include "greedy_placement.h"

#include <queue>
#include <utility>

#include "lib.h"

namespace woops
{
using Pair = std::pair<Hostid, size_t>;
class PairComparator
{
public:
    bool operator()(Pair& l, Pair& r) {
        return l.second > r.second;
    }
private:
};

void GreedyPlacement::Decision() {
    std::priority_queue<Pair, std::vector<Pair>, PairComparator> sizes;
    for (Hostid h = 0; h < Lib::NumHosts(); ++h) {
        sizes.emplace(h, 0);
    }
    for (auto& config: Lib::TableConfigs()) {
        auto size = sizes.top();
        sizes.pop();
        table_to_partitions_[config.id][size.first] = Partition{0, config.size};
        size.second += config.size * config.element_size;
        sizes.push(size);
    }

}
    
} /* woops */ 
