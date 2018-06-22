#ifndef WOOPS_CONSISTENCY_TRADITIONAL_SSP_CONSISTENCY_H_
#define WOOPS_CONSISTENCY_TRADITIONAL_SSP_CONSISTENCY_H_

#include "ssp_consistency.h"

namespace woops
{
class TraditionalSSPConsistency: public SSPConsistency
{
public:
    TraditionalSSPConsistency (Iteration staleness);
    void BeforeClock(Iteration iteration) override;

private:
    /* data */
};    
} /* woops */ 


#endif /* end of include guard: WOOPS_CONSISTENCY_TRADITIONAL_SSP_CONSISTENCY_H_ */
