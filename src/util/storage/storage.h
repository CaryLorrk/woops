#ifndef WOOPS_UTIL_STORAGE_STORAGE_H_
#define WOOPS_UTIL_STORAGE_STORAGE_H_

#include <string>
#include <map>
#include "util/typedef.h"
#include "util/placement/placement.h"

namespace woops
{

class Storage
{
public:
    virtual ~Storage() {}
    virtual void Deserialize(const Bytes& bytes) = 0;
    virtual void Zerofy() = 0;
    virtual Bytes Serialize() const = 0;
    virtual Bytes Encode() = 0;
    virtual std::map<Hostid, Bytes> Encode(const Placement::Partitions& partitions) = 0;
    virtual void Decode(Hostid host, const Bytes& bytes) = 0;
    virtual void Decode(const Bytes& bytes, const Placement::Partition& partition) = 0;
    virtual void Assign(const Storage& data) = 0;
    virtual void Update(const Storage& delta) = 0;
    virtual std::string ToString() const = 0;
};
    
} /* woops */ 

#endif /* end of include guard: WOOPS_UTIL_STORAGE_STORAGE_H_ */
