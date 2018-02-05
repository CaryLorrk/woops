#ifndef WOOPS_UTIL_PLACEMENT_ROUND_ROBIN_PLACEMENT_H_
#define WOOPS_UTIL_PLACEMENT_ROUND_ROBIN_PLACEMENT_H_

#include <unordered_map>

#include "placement.h"

namespace woops
{
class RoundRobinPlacement: public Placement
{
public:
    void Decision() override;
}; 
} /* woops */ 



#endif /* end of include guard: WOOPS_UTIL_PLACEMENT_ROUND_ROBIN_PLACEMENT_H_ */
