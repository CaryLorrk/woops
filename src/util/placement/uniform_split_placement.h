#ifndef WOOPS_UTIL_PLACEMENT_UNIFORM_SPLIT_PLACEMENT_H_
#define WOOPS_UTIL_PLACEMENT_UNIFORM_SPLIT_PLACEMENT_H_

#include "placement.h"

namespace woops
{

class UniformSplitPlacement: public Placement
{
public:
    void RegisterTable(const TableConfig& config) override;
    void Decision() override;
private:
    std::vector<TableConfig> configs_;
};
    
} /* woops */ 


#endif /* end of include guard: WOOPS_UTIL_PLACEMENT_UNIFORM_SPLIT_PLACEMENT_H_ */
