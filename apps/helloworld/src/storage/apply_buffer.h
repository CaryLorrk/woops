#ifndef APPLY_BUFFER_H_QJDYACGF
#define APPLY_BUFFER_H_QJDYACGF

#include "util/storage/dense_storage.h"

namespace woops
{
template<typename T>
class ApplyBuffer: public DenseStorage<T>
{
public:
    ApplyBuffer (size_t size): DenseStorage<T>(size) {}
};
} /* woops */ 


#endif /* end of include guard: APPLY_BUFFER_H_QJDYACGF */
