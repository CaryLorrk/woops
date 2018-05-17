#ifndef WOOPS_CONSISTENCY_AGGRESIVE_SSP_CONSISTENCY_H_
#define WOOPS_CONSISTENCY_AGGRESIVE_SSP_CONSISTENCY_H_

#include "ssp_consistency.h"

namespace woops
{

class AggressiveSSPConsistency: public SSPConsistency
{
public:
    AggressiveSSPConsistency(Iteration staleness);
    void ClientUpdate(Tableid id, const Storage& storage, Iteration iteration) override;

};

} /* woops */ 

#endif /* end of include guard: WOOPS_CONSISTENCY_AGGRESIVE_SSP_CONSISTENCY_H_ */
