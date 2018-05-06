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
            MAYBE_UNUSED const Placement::Partition& partition) override {
        std::lock_guard<std::mutex> lock(this->mu_);
        auto it = bytes.begin();
        while (it != bytes.end()) {
            ParamIndex idx = *reinterpret_cast<const ParamIndex*>(&(*it));
            std::advance(it, sizeof(ParamIndex)/sizeof(Byte));
            this->data_[idx] += *reinterpret_cast<const T*>(&(*it));
            std::advance(it, sizeof(T)/sizeof(Byte));
        }
    }
};
} /* woops */ 


#endif /* end of include guard: APPLY_BUFFER_H_QJDYACGF */
