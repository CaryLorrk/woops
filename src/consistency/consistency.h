#ifndef WOOPS_CONSISTENCY_CONSISTENCY_H_
#define WOOPS_CONSISTENCY_CONSISTENCY_H_

#include "util/typedef.h"
#include "util/storage/storage.h"

namespace woops
{
class Consistency
{
public:
    virtual void ClientSync(
            MAYBE_UNUSED Tableid id,
            MAYBE_UNUSED Iteration iteration) {}

    virtual void ClientUpdate(
            MAYBE_UNUSED Tableid id,
            MAYBE_UNUSED const Storage& storage,
            MAYBE_UNUSED Iteration iteration) {}

    virtual void ServerPushHandler(
            MAYBE_UNUSED Hostid server,
            MAYBE_UNUSED Tableid id,
            MAYBE_UNUSED Iteration iteration,
            MAYBE_UNUSED const Bytes& bytes,
            MAYBE_UNUSED Iteration iteration_now) {}

    virtual Iteration GetServerData(
            MAYBE_UNUSED Hostid client,
            MAYBE_UNUSED Tableid id,
            MAYBE_UNUSED Iteration iteration) { return 0; }

    virtual void ClientPushHandler(
            MAYBE_UNUSED Hostid client, 
            MAYBE_UNUSED Tableid id, 
            MAYBE_UNUSED Iteration iteration, 
            MAYBE_UNUSED const Bytes& bytes) {}


    virtual ~Consistency (){}
private:
    
}; 
} /* woops */ 


#endif /* end of include guard: WOOPS_CONSISTENCY_CONSISTENCY_H_ */
