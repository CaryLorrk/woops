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
    enum class DecodingType {ASSIGN, UPDATE};
    virtual ~Storage() {}
    virtual void Zerofy() = 0;
    virtual Bytes Encode() const = 0;
    virtual std::map<Hostid, Bytes> Encode(const Placement::Partitions& partitions) const = 0;
    virtual void Decode(const Bytes& bytes, size_t offset = 0, DecodingType = DecodingType::UPDATE) = 0;
    virtual void Assign(const Storage& data, size_t offset = 0) = 0;
    virtual void Update(const Storage& delta, size_t offset = 0) = 0;
    virtual std::string ToString() const = 0;
};
    
} /* woops */ 

#endif /* end of include guard: WOOPS_UTIL_STORAGE_STORAGE_H_ */
