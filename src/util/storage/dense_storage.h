#ifndef WOOPS_UTIL_STROAGE_DENSE_STORAGE_H_
#define WOOPS_UTIL_STROAGE_DENSE_STORAGE_H_

#include <cstring>
#include <sstream>
#include <memory>
#include <mutex>

#include "storage.h"

namespace woops
{
template<typename T>
class DenseStorage: public Storage
{
public:
    DenseStorage(int size);

    void Zerofy() override;
    Bytes Encode() const override;
    std::map<Hostid, Bytes> Encode(const Placement::Partitions& partitions) const override;
    void Decode(const Bytes& bytes, size_t offset, DecodingType decoding_type = DecodingType::UPDATE) override;
    void Assign(const T* data, size_t size = 0, size_t offset = 0);
    void Assign(const Bytes& bytes, size_t offset = 0) override;
    void Update(const Bytes& bytes, size_t offset = 0) override;
    std::string ToString() const override;

private:
    std::vector<T> data_;
    std::mutex mu_;
};

template<typename T>
DenseStorage<T>::DenseStorage(int size):
    data_(size)
{
}

template<typename T>
void DenseStorage<T>::Zerofy() {
    std::lock_guard<std::mutex> lock(mu_);
    std::fill(data_.begin(), data_.end(), 0);
}

template<typename T>
Bytes DenseStorage<T>::Encode() const {
    int byte_size = data_.size() * sizeof(T);
    Bytes ret(byte_size, 0);
    memcpy(&ret[0], data_.data(), byte_size);
    return ret;
}

template<typename T>
std::map<Hostid, Bytes> DenseStorage<T>::Encode(const Placement::Partitions& partitions) const {
    std::map<Hostid, Bytes> ret;
    for (auto& server_part: partitions) {
        Hostid server = server_part.first;
        const Placement::Partition& part = server_part.second;
        ret[server] = std::string{(char*)&data_[part.begin], (char*)&data_[part.end]};
    }
    return ret;
}

template<typename T>
void DenseStorage<T>::Decode(const Bytes& bytes, size_t offset, DecodingType decoding_type) {
    switch (decoding_type) {
    case DecodingType::ASSIGN:
        Assign(bytes, offset);
        break;
    case DecodingType::UPDATE:
        Update(bytes, offset);
        break;
    default:
        LOG(FATAL) << "Unknown decoding type.";

    }
}

template<typename T>
void DenseStorage<T>::Assign(const T* data, size_t size, size_t offset) {
    std::lock_guard<std::mutex> lock(mu_);
    if (size == 0) size = data_.size();
    std::memcpy(&data_[offset], data, size * sizeof(T));
}

template<typename T>
void DenseStorage<T>::Assign(const Bytes& bytes, size_t offset) {
    std::lock_guard<std::mutex> lock(mu_);
    const T* data = reinterpret_cast<const T*>(bytes.data());
    std::memcpy(&data_[offset], data, bytes.size());
}

template<typename T>
void DenseStorage<T>::Update(const Bytes& bytes, size_t offset) {
    std::lock_guard<std::mutex> lock(mu_);
    const T* delta = reinterpret_cast<const T*>(bytes.data());
    size_t size = bytes.size() / sizeof(T);
    for (size_t i = offset; i < offset + size; ++i) {
        data_[i] += delta[i];
    }
}

template<typename T>
std::string DenseStorage<T>::ToString() const {
    std::stringstream ss;
    for(size_t i = 0; i < data_.size(); ++i) {
        ss << data_[i] << " ";
    }
    return ss.str();
}
    
} /* woops */ 

#endif /* end of include guard: WOOPS_UTIL_STROAGE_DENSE_STORAGE_H_ */
