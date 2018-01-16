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
    virtual ~DenseStorage();

    void Zerofy() override;
    const void* Serialize() const override;
    size_t GetSize() const override;
    void Assign(const void* data, size_t offset = 0, size_t size = -1) override;
    std::map<Hostid, Bytes> Encoding(const Placement::Partitions& partitions) const override;
    void Update(const void* delta) override;
    std::string ToString() const override;

private:
    size_t size_;
    std::unique_ptr<T[]> data_;
    std::mutex mu_;
};

template<typename T>
DenseStorage<T>::DenseStorage(size_t size):
    size_(size),
    data_(new T[size])
{
}

template<typename T>
DenseStorage<T>::~DenseStorage() {}

template<typename T>
void DenseStorage<T>::Zerofy() {
    std::lock_guard<std::mutex> lock(mu_);
    std::memset(data_.get(), 0, sizeof(T) * size_);
}

template<typename T>
const void* DenseStorage<T>::Serialize() const {
    return data_.get();
}

template<typename T>
std::map<Hostid, Bytes> DenseStorage<T>::Encoding(const Placement::Partitions& partitions) const {
    std::map<Hostid, Bytes> ret;
    for (auto& server_part: partitions) {
        Hostid server = server_part.first;
        const Placement::Partition& part = server_part.second;
        ret[server] = std::string{(char*)&data_[part.begin], (char*)&data_[part.end]};
    }
    return ret;
}

template<typename T>
size_t DenseStorage<T>::GetSize() const {
    return size_ * sizeof(T);
}

template<typename T>
void DenseStorage<T>::Assign(const void* data, size_t offset, size_t size) {
    std::lock_guard<std::mutex> lock(mu_);
    if (size == -1) size = size_;
    T* dst = (T*)data_.get() + offset;
    std::memcpy(dst, data, sizeof(T) * size);
}

template<typename T>
void DenseStorage<T>::Update(const void* delta) {
    std::lock_guard<std::mutex> lock(mu_);
    const T* t_delta = static_cast<const T*>(delta);
    for (size_t i = 0; i < size_; ++i) {
        data_[i] += t_delta[i];
    }
}

template<typename T>
std::string DenseStorage<T>::ToString() const {
    std::stringstream ss;
    for(size_t i = 0; i < size_; ++i) {
        ss << data_[i] << " ";
    }
    return ss.str();
}
    
} /* woops */ 

#endif /* end of include guard: WOOPS_UTIL_STROAGE_DENSE_STORAGE_H_ */
