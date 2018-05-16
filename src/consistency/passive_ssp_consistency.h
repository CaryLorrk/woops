#ifndef WOOPS_CONSISTENCY_PASSIVE_SSP_CONSISTENCY_H_
#define WOOPS_CONSISTENCY_PASSIVE_SSP_CONSISTENCY_H_

#include "consistency.h"

namespace woops
{
class PassiveSSPConsistency: public Consistency
{
public: 
    PassiveSSPConsistency (Iteration staleness);
    void ClientSync(Tableid id, Iteration iteration) override;
    void ClientUpdate(Tableid id, const Storage& storage, Iteration iteration) override;
    void ServerPushHandler(Hostid server, Tableid id, Iteration iteration, const Bytes& bytes, Iteration iteration_now)  override;

    Iteration GetServerData(Hostid client, Tableid id, Iteration iteration) override;
    void ClientPushHandler(Hostid client, Tableid id, Iteration iteration, const Bytes& bytes)  override;
private:
    Iteration staleness_;
}; 
} /* woops */ 

#endif /* end of include guard: WOOPS_CONSISTENCY_PASSIVE_SSP_CONSISTENCY_H_ */
