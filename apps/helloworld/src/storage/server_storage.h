#ifndef SERVER_STORAGE_H_Q4KYZGRA
#define SERVER_STORAGE_H_Q4KYZGRA

#include "util/storage/dense_storage.h"

namespace woops
{
template<typename T>
class ServerStorage: public DenseStorage<T>
{
public:
    ServerStorage (size_t size): DenseStorage<T>(size) {}
};
 
} /* woops */ 



#endif /* end of include guard: SERVER_STORAGE_H_Q4KYZGRA */
