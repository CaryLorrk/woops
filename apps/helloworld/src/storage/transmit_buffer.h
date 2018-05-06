#ifndef TRANSMIT_BUFFER_H_FVL8G2DT
#define TRANSMIT_BUFFER_H_FVL8G2DT

#include "util/storage/dense_storage.h"

namespace woops
{
template<typename T>
class TransmitBuffer: public DenseStorage<T>
{
public:
    TransmitBuffer(size_t size): DenseStorage<T>(size) {}
    std::map<Hostid, Bytes> Encode(const Placement::Partitions& partitions) override {
        std::lock_guard<std::mutex> lock(this->mu_);
        std::map<Hostid, Bytes> ret;
        for (auto&& server_part: partitions) {
            Hostid server = server_part.first;
            const Placement::Partition& part = server_part.second;
            ret[server] = Bytes{(Byte*)&this->data_[part.begin], (Byte*)&this->data_[part.end]};
        }
        this->zerofy();
        return ret;
    }
};
}


#endif /* end of include guard: TRANSMIT_BUFFER_H_FVL8G2DT */
