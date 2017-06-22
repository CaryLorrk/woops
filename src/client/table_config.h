#ifndef WOOPS_CLIENT_TABLE_CONFIG_H_
#define WOOPS_CLIENT_TABLE_CONFIG_H_

#include <string>
#include <memory>
#include <functional>

#include "common/storage/storage.h"

namespace woops
{

using StorageConstructor = std::function<std::unique_ptr<Storage>(size_t size)> ;

struct TableConfig {
    std::string name;
    size_t size;
    size_t element_size;
    StorageConstructor cache_constructor;
    StorageConstructor server_storage_constructor;
};

} /* woops */ 

#endif /* end of include guard: WOOPS_CLIENT_TABLE_CONFIG_H_ */
