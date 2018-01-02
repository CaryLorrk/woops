#ifndef WOOPS_PLACEMENT_PLACEMENT_H_
#define WOOPS_PLACEMENT_PLACEMENT_H_

#include <vector>

#include "util/config/table_config.h"

namespace woops
{
class Placement
{
public:
    virtual void RegisterTable(const TableConfig& config) = 0;
    virtual void Decision() = 0;
    virtual void Split() = 0;
};    
} /* woops */ 

#endif /* end of include guard: WOOPS_PLACEMENT_PLACEMENT_H_ */
