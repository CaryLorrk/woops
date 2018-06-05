#ifndef WOOPS_CONSISTENCY_SSP_CONSISTENCY_H_
#define WOOPS_CONSISTENCY_SSP_CONSISTENCY_H_

#include "consistency.h"

#include "util/typedef.h"

namespace woops
{
class SSPConsistency: public Consistency
{
public:
    SSPConsistency (Iteration staleness);
    void ClientSync(Tableid id, Iteration iteration) override;
    void AfterClientUpdate(Tableid id, const Storage& storage, Iteration iteration) override;
    void AfterServerPushHandler(Hostid server, Tableid id, Iteration iteration, const Bytes& bytes, Iteration iteration_now)  override;

    Iteration GetServerData(Hostid client, Tableid id, Iteration iteration) override;
    void ClientPushHandler(Hostid client, Tableid id, Iteration iteration, const Bytes& bytes)  override;

protected:
    Iteration staleness_;
};
    
} /* woops */ 


#endif /* end of include guard: WOOPS_CONSISTENCY_SSP_CONSISTENCY_H_ */
