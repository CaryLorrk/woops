#ifndef WOOPS_CONSISTENCY_ADAPTIVE_CONSISTENCY_H_
#define WOOPS_CONSISTENCY_ADAPTIVE_CONSISTENCY_H_

#include "consistency.h"

namespace woops
{

class AdaptiveConsistency: public Consistency
{
public:
    AdaptiveConsistency(int threshold);

private:
    int threshold_;
};
    
} /* woops */ 

#endif /* end of include guard: WOOPS_CONSISTENCY_ADAPTIVE_CONSISTENCY_H_ */
