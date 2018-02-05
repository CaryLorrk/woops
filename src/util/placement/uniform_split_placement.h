#ifndef WOOPS_UTIL_PLACEMENT_UNIFORM_SPLIT_PLACEMENT_H_
#define WOOPS_UTIL_PLACEMENT_UNIFORM_SPLIT_PLACEMENT_H_

#include <unordered_map>

#include "placement.h"

namespace woops
{

class UniformSplitPlacement: public Placement
{
public:
    void Decision() override;
};
    
} /* woops */ 


#endif /* end of include guard: WOOPS_UTIL_PLACEMENT_UNIFORM_SPLIT_PLACEMENT_H_ */
