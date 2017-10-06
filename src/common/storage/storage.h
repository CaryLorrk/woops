#ifndef WOOPS_COMMON_STORAGE_STORAGE_H_
#define WOOPS_COMMON_STORAGE_STORAGE_H_

#include <string>

namespace woops
{

class Storage
{
public:
    virtual ~Storage() {}
    virtual void Zerofy() = 0;
    virtual const void* Serialize() const = 0;
    virtual size_t GetByteSize() const = 0;
    virtual void Assign(const void* data, size_t offset = 0, size_t size = -1) = 0;
    virtual void Update(const void* delta) = 0;
    virtual std::string ToString() const = 0;
};
    
} /* woops */ 

#endif /* end of include guard: WOOPS_COMMON_STORAGE_STORAGE_H_ */
