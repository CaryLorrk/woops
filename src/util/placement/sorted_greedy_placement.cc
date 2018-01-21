#include "sorted_greedy_placement.h"

#include <algorithm>
#include <queue>
#include <utility>

namespace woops
{
using Pair = std::pair<Hostid, size_t>;
void SortedGreedyPlacement::Initialize(const WoopsConfig& config) {
    num_hosts_ = config.hosts.size();
}

class PairComparator
{
public:
    bool operator()(Pair& l, Pair& r) {
        return l.second > r.second;
    }
private:
};

void SortedGreedyPlacement::Decision() {
    std::sort(configs_.begin(), configs_.end(),
            [](const TableConfig& l, const TableConfig& r) {
                return l.size * l.element_size < r.size * r.element_size;
            });
    std::priority_queue<Pair, std::vector<Pair>, PairComparator> sizes;
    for (Hostid h = 0; h < num_hosts_; ++h) {
        sizes.emplace(h, 0);
    }
    for (auto& config: configs_) {
        auto size = sizes.top();
        sizes.pop();
        table_to_partitions_[config.id][size.first] = Partition{0, config.size};
        size.second += config.size * config.element_size;
        sizes.push(size);
    }

}
    
} /* woops */ 
