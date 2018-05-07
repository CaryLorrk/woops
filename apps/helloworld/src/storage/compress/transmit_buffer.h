#ifndef TRANSMIT_BUFFER_H_FVL8G2DT
#define TRANSMIT_BUFFER_H_FVL8G2DT

#include <numeric>

#include "util/storage/dense_storage.h"

namespace woops
{
template<typename T>
class TransmitBuffer: public DenseStorage<T>
{
public:
    TransmitBuffer(size_t size): DenseStorage<T>(size) {}
    std::map<Hostid, Bytes> Encode(const Placement::Partitions& partitions) override {
        constexpr int COMPRESSION_RATIO = 2;
        std::lock_guard<std::mutex> lock(this->mu_);
        std::map<Hostid, Bytes> ret;
        std::vector<ParamIndex> index(this->data_.size());
        std::iota(index.begin(), index.end(), 0);
        auto&& middle = std::next(index.begin(),
                (this->data_.size() + COMPRESSION_RATIO - 1) / COMPRESSION_RATIO);
        std::partial_sort(index.begin(), middle, index.end(), [this](const T& lhs, const T& rhs) {
            auto&& data = this->data_;
            return std::abs(data[lhs]) > std::abs(data[rhs]);
        });
        
        std::sort(index.begin(), middle);

        auto&& kv = partitions.begin();
        for (auto it = index.begin(); it != middle; ++it) {
            const ParamIndex& idx = *it;
            T& val = this->data_[idx];
            while (idx >= kv->second.end) ++kv;
            Hostid server = kv->first;
            ret[server].append((Byte*)&(idx), (Byte*)(&(idx) + 1));
            ret[server].append((Byte*)&(val), (Byte*)(&(val) + 1));
            val = 0;
        }

        return ret;
    }
private:
};

    
}


#endif /* end of include guard: TRANSMIT_BUFFER_H_FVL8G2DT */
