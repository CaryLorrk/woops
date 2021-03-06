#ifndef WOOPS_CLIENT_CONFIG_TABLE_CONFIG_H_
#define WOOPS_CLIENT_CONFIG_TABLE_CONFIG_H_

#include <string>
#include <memory>
#include <functional>

#include "util/typedef.h"

namespace woops
{
class Storage;
using StorageConstructor = std::function<Storage*(Tableid)> ;

struct TableConfig {
    Tableid id;
    size_t size;
    size_t element_size;
    StorageConstructor worker_storage_constructor;
    StorageConstructor server_storage_constructor;
    StorageConstructor apply_buffer_constructor;
    StorageConstructor transmit_buffer_constructor;
};

} /* woops */ 

#endif /* end of include guard: WOOPS_CONFIG_TABLE_CONFIG_H_ */
