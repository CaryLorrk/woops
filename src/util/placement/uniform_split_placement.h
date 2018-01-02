#ifndef WOOPS_UTIL_PLACEMENT_UNIFORM_SPLIT_PLACEMENT_H_
#define WOOPS_UTIL_PLACEMENT_UNIFORM_SPLIT_PLACEMENT_H_

#include <unordered_map>

#include "placement.h"

namespace woops
{

class UniformSplitPlacement: public Placement
{
public:
    void RegisterTable(const TableConfig& config) override;
    void Decision() override;
    void Split() override;
private:
    std::vector<TableConfig> configs_;
    std::unordered_map<int, int> begins;
};
    
} /* woops */ 


#endif /* end of include guard: WOOPS_UTIL_PLACEMENT_UNIFORM_SPLIT_PLACEMENT_H_ */
