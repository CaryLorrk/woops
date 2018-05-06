#ifndef SERVER_STORAGE_H_Q4KYZGRA
#define SERVER_STORAGE_H_Q4KYZGRA

#include "util/storage/dense_storage.h"

#include "lib.h"

namespace woops
{
template<typename T>
class ServerStorage: public DenseStorage<T>
{
public:
    ServerStorage (size_t size): DenseStorage<T>(size) {}
    Bytes Encode() override {
        std::lock_guard<std::mutex> lock(this->mu_);
        Bytes ret;
        for (ParamIndex idx = 0; idx < (ParamIndex)this->data_.size(); ++idx) {
            const T& val = this->data_[idx];
            if (!val) continue;
            ret.append((Byte*)&(idx), (Byte*)(&(idx) + 1));
            ret.append((Byte*)&(val), (Byte*)(&(val) + 1));
        }
        this->zerofy();
        return ret;
    }
    void Decode(
            MAYBE_UNUSED Hostid host,
            const Bytes& bytes) {
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



#endif /* end of include guard: SERVER_STORAGE_H_Q4KYZGRA */
