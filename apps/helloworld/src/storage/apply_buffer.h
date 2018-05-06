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
    void Decode(
            const Bytes& bytes,
            const Placement::Partition& partition) override {
        const T* data = reinterpret_cast<const T*>(bytes.data());
        size_t size = bytes.size() / sizeof(T);
        std::lock_guard<std::mutex> lock(this->mu_);
        this->update(data, size, partition.begin);
    }
};
} /* woops */ 


#endif /* end of include guard: APPLY_BUFFER_H_QJDYACGF */
