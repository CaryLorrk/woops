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

    void Zerofy() override;
    Bytes Serialize() const override;
    void Deserialize(const Bytes& bytes) override;
    Bytes Encode() override;
    std::map<Hostid, Bytes> Encode(const Placement::Partitions& partitions) override;
    void Decode(Hostid host, const Bytes& bytes) override;
    void Decode(const Bytes& bytes, const Placement::Partition& partition) override;
    void Assign(const Storage& data) override;
    void Update(const Storage& delta) override;
    std::string ToString() const override;

protected:
    std::vector<T> data_;
    mutable std::mutex mu_;

    void zerofy();
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
void DenseStorage<T>::Zerofy() {
    std::lock_guard<std::mutex> lock(mu_);
    zerofy();
}

template<typename T>
Bytes DenseStorage<T>::Serialize() const {
    std::lock_guard<std::mutex> lock(mu_);
    return Bytes{(Byte*)data_.data(), data_.size() * sizeof(T)};
}

template<typename T>
void DenseStorage<T>::Deserialize(const Bytes& bytes) {
    std::lock_guard<std::mutex> lock(mu_);
    const T* data = reinterpret_cast<const T*>(bytes.data());
    size_t size = bytes.size() / sizeof(T);
    assign(data, size);
}

template<typename T>
Bytes DenseStorage<T>::Encode() {
    Bytes ret;
    LOG(FATAL) << "Unimplemented";
    return ret;
}

template<typename T>
std::map<Hostid, Bytes> DenseStorage<T>::Encode(
        MAYBE_UNUSED const Placement::Partitions& partitions) {
    std::map<Hostid, Bytes> ret;
    LOG(FATAL) << "Unimplemented";
    return ret;
}

template<typename T>
void DenseStorage<T>::Decode(
        MAYBE_UNUSED Hostid host,
        MAYBE_UNUSED const Bytes& bytes) {
    LOG(FATAL) << "Unimplemented";
}

template<typename T>
void DenseStorage<T>::Decode(
        MAYBE_UNUSED const Bytes& bytes,
        MAYBE_UNUSED const Placement::Partition& partition) {
    LOG(FATAL) << "Unimplemented";
}

template<typename T>
void DenseStorage<T>::Assign(const Storage& data) {
    auto&& t_data = reinterpret_cast<const DenseStorage<T>&>(data); 
    std::lock_guard<std::mutex> lock(mu_);
    assign(t_data.data_.data(), t_data.data_.size());
}

template<typename T>
void DenseStorage<T>::Update(const Storage& delta) {
    auto&& t_delta = reinterpret_cast<const DenseStorage<T>&>(delta); 
    std::lock_guard<std::mutex> lock(mu_);
    update(t_delta.data_.data(), t_delta.data_.size());
}

template<typename T>
void DenseStorage<T>::zerofy() {
    std::fill(data_.begin(), data_.end(), 0);
}

template<typename T>
void DenseStorage<T>::assign(const T* data, size_t size, size_t offset) {
    std::copy(data, data + size, std::next(data_.begin(), offset));
}

template<typename T>
void DenseStorage<T>::update(const T* delta, size_t size, size_t offset) {
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
