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
    DenseStorage(size_t size);
    DenseStorage(const std::vector<T>& data);
    DenseStorage(std::vector<T>&& data);

    void Sync(const Bytes& bytes) override;
    void Zerofy() override;
    Bytes Encode() const override;
    std::map<Hostid, Bytes> Encode(const Placement::Partitions& partitions) const override;
    void Decode(const Bytes& bytes, size_t offset) override;
    void Assign(const Storage& data, size_t offset = 0) override;
    void Update(const Storage& delta, size_t offset = 0) override;
    std::string ToString() const override;

protected:
    std::vector<T> data_;
    mutable std::mutex mu_;

    void assign(const T* data, size_t size, size_t offset = 0);
    void update(const T* delta, size_t size, size_t offset = 0);
};

template<typename T>
DenseStorage<T>::DenseStorage(size_t size):data_(size) {}

template<typename T>
DenseStorage<T>::DenseStorage(const std::vector<T>& data) {
    data_ = data;
}

template<typename T>
DenseStorage<T>::DenseStorage(std::vector<T>&& data) {
    data_ = std::move(data);
}

template<typename T>
void DenseStorage<T>::Sync(const Bytes& bytes) {
    const T* data = reinterpret_cast<const T*>(bytes.data());
    size_t size = bytes.size() / sizeof(T);
    assign(data, size);
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
    for (auto&& server_part: partitions) {
        Hostid server = server_part.first;
        const Placement::Partition& part = server_part.second;
        ret[server] = std::string{(char*)&data_[part.begin], (char*)&data_[part.end]};
    }
    return ret;
}

template<typename T>
void DenseStorage<T>::Decode(const Bytes& bytes, size_t offset) {
    const T* data = reinterpret_cast<const T*>(bytes.data());
    size_t size = bytes.size() / sizeof(T);
    update(data, size, offset);
}

template<typename T>
void DenseStorage<T>::Assign(const Storage& data, size_t offset) {
    auto&& t_data = reinterpret_cast<const DenseStorage<T>&>(data); 
    assign(t_data.data_.data(), t_data.data_.size(), offset);
}

template<typename T>
void DenseStorage<T>::Update(const Storage& delta, size_t offset) {
    auto&& t_delta = reinterpret_cast<const DenseStorage<T>&>(delta); 
    update(t_delta.data_.data(), t_delta.data_.size(), offset);
}

template<typename T>
void DenseStorage<T>::assign(const T* data, size_t size, size_t offset) {
    std::lock_guard<std::mutex> lock(mu_);
    std::copy(data, data + size, std::next(data_.begin(), offset));
}

template<typename T>
void DenseStorage<T>::update(const T* delta, size_t size, size_t offset) {
    std::lock_guard<std::mutex> lock(mu_);
    auto&& begin_it = std::next(data_.begin(), offset);
    std::transform(delta, delta + size, begin_it, begin_it, std::plus<T>());
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
