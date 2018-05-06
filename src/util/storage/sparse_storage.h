#ifndef WOOPS_UTIL_STORAGE_SPARSE_STORAGE_H_
#define WOOPS_UTIL_STORAGE_SPARSE_STORAGE_H_

#include "storage.h"

#include <map>
#include <mutex>

namespace woops
{

template<typename T>
class SparseStorage: public Storage
{
public:
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
    std::map<ParamIndex, T> data_;
    mutable std::mutex mu_;

    void zerofy();
    Bytes serialize(
        typename decltype(data_)::const_iterator first,
        typename decltype(data_)::const_iterator last) const;

    void deserialize(const Bytes& bytes);
};

template<typename T>
void SparseStorage<T>::Zerofy() {
    std::lock_guard<std::mutex> lock(mu_);
    zerofy();
}

template<typename T>
Bytes SparseStorage<T>::Serialize() const {
    std::lock_guard<std::mutex> lock(mu_);
    return serialize(data_.begin(), data_.cend());
}

template<typename T>
void SparseStorage<T>::Deserialize(const Bytes& bytes) {
    std::lock_guard<std::mutex> lock(mu_);
    deserialize(bytes);
}

template<typename T>
Bytes SparseStorage<T>::Encode() {
    Bytes ret;
    LOG(FATAL) << "Unimplemented";
    return ret;
}

template<typename T>
std::map<Hostid, Bytes> SparseStorage<T>::Encode(
        MAYBE_UNUSED const Placement::Partitions& partitions) {
    std::map<Hostid, Bytes> ret;
    LOG(FATAL) << "Unimplemented";
    return ret;
}

template<typename T>
void SparseStorage<T>::Decode(
        MAYBE_UNUSED Hostid host,
        MAYBE_UNUSED const Bytes& bytes) {
    LOG(FATAL) << "Unimplemented";
}

template<typename T>
void SparseStorage<T>::Decode(
        MAYBE_UNUSED const Bytes& bytes,
        MAYBE_UNUSED const Placement::Partition& partition) {
    LOG(FATAL) << "Unimplemented";
}

template<typename T>
void SparseStorage<T>::Assign(
        MAYBE_UNUSED const Storage& data) {
    LOG(FATAL) << "Unimplemented";
}

template<typename T>
void SparseStorage<T>::Update(
        MAYBE_UNUSED const Storage& delta) {
    LOG(FATAL) << "Unimplemented";
}

template<typename T>
std::string SparseStorage<T>::ToString() const {
    LOG(FATAL) << "Unimplemented";
    return "";
} 


template<typename T>
void SparseStorage<T>::zerofy() {
    data_.clear();
}

template<typename T>
Bytes SparseStorage<T>::serialize(
        typename decltype(data_)::const_iterator first,
        typename decltype(data_)::const_iterator last) const {
    Bytes ret;
    for (auto it = first; it != last; std::advance(it, 1)) {
        const ParamIndex idx = it->first;
        const T& val = it->second;
        ret.append((Byte*)&(idx), (Byte*)(&(idx) + 1));
        ret.append((Byte*)&(val), (Byte*)(&(val) + 1));
    }
    return ret;
}

template<typename T>
void SparseStorage<T>::deserialize(const Bytes& bytes) {
    auto it = bytes.begin();
    while (it != bytes.end()) {
        ParamIndex idx = *reinterpret_cast<const ParamIndex*>(&(*it));
        std::advance(it, sizeof(ParamIndex)/sizeof(Byte));
        this->data_[idx] += *reinterpret_cast<const T*>(&(*it));
        std::advance(it, sizeof(T)/sizeof(Byte));
    }
}

    
} /* woops */ 


#endif /* end of include guard: WOOPS_UTIL_STORAGE_SPARSE_STORAGE_H_ */
