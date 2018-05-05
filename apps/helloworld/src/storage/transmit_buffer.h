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
        std::map<Hostid, Bytes> ret;
        for (auto&& server_part: partitions) {
            Hostid server = server_part.first;
            const Placement::Partition& part = server_part.second;
            ret[server] = std::string{(char*)&this->data_[part.begin], (char*)&this->data_[part.end]};
        }
        this->Zerofy();
        return ret;
    }
};
}


#endif /* end of include guard: TRANSMIT_BUFFER_H_FVL8G2DT */
