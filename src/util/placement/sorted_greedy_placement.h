#ifndef WOOPS_UTIL_PLACEMENT_SORTED_GREEDY_PLACEMENT_H_
#define WOOPS_UTIL_PLACEMENT_SORTED_GREEDY_PLACEMENT_H_

#include "placement.h"

namespace woops
{
class SortedGreedyPlacement: public Placement
{
public:
    void Decision() override;
};
    
} /* woops */ 



#endif /* end of include guard: WOOPS_UTIL_PLACEMENT_SORTED_GREEDY_PLACEMENT_H_ */
