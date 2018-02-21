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
    virtual void Zerofy() = 0;
    virtual const void* Serialize() const = 0;
    virtual std::map<Hostid, Bytes> Encoding(const Placement::Partitions& partitions) const = 0;
    virtual void Decoding(const Bytes& bytes, size_t offset = 0, size_t size = -1) = 0;
    virtual size_t GetSize() const = 0;
    virtual void Assign(const void* data, size_t offset = 0, size_t size = -1) = 0;
    virtual void Update(const void* delta) = 0;
    virtual std::string ToString() const = 0;
};
    
} /* woops */ 

#endif /* end of include guard: WOOPS_UTIL_STORAGE_STORAGE_H_ */
