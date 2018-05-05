#ifndef WOOPS_CLIENT_CONFIG_TABLE_CONFIG_H_
#define WOOPS_CLIENT_CONFIG_TABLE_CONFIG_H_

#include <string>
#include <memory>
#include <functional>

#include "util/typedef.h"

namespace woops
{
class Storage;
using StorageConstructor = std::function<std::unique_ptr<Storage>()> ;

struct TableConfig {
    Tableid id;
    size_t size;
    size_t element_size;
    StorageConstructor client_storage_constructor;
    StorageConstructor server_storage_constructor;
    StorageConstructor apply_buffer_constructor;
};

} /* woops */ 

#endif /* end of include guard: WOOPS_CONFIG_TABLE_CONFIG_H_ */
