#ifndef WOOPS_UTIL_PLACEMENT_SORTED_GREEDY_PLACEMENT_H_
#define WOOPS_UTIL_PLACEMENT_SORTED_GREEDY_PLACEMENT_H_

#include "placement.h"

namespace woops
{
class SortedGreedyPlacement: public Placement
{
public:
    void Initialize(const WoopsConfig& config) override;
    void Decision() override;

private:
    size_t num_hosts_;
};
    
} /* woops */ 



#endif /* end of include guard: WOOPS_UTIL_PLACEMENT_SORTED_GREEDY_PLACEMENT_H_ */
