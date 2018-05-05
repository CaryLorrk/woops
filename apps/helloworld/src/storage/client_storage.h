#ifndef CLIENT_STORAGE_H_HD0CJRSB
#define CLIENT_STORAGE_H_HD0CJRSB

#include "util/storage/dense_storage.h"

namespace woops
{

template<typename T>
class ClientStorage: public DenseStorage<T>
{
public:
    ClientStorage(size_t size): DenseStorage<T>(size) {}
};
    
} /* woops */ 


#endif /* end of include guard: CLIENT_STORAGE_H_HD0CJRSB */
