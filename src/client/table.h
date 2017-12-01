#ifndef WOOPS_CLIENT_TABLE_H_
#define WOOPS_CLIENT_TABLE_H_

#include <memory>
#include <mutex>
#include <condition_variable>

#include <grpc++/grpc++.h>

#include "util/storage/storage.h"
#include "util/protobuf/ps_service.pb.h"
#include "util/config/table_config.h"

namespace woops
{
struct Table {
    std::unique_ptr<Storage> cache; 
    size_t size;
    size_t element_size;
    std::mutex  mu;
    std::condition_variable cv;
    std::vector<int> iterations;
    std::vector<int> host_ends;
};
} /* woops */ 



#endif /* end of include guard: WOOPS_CLIENT_TABLE_H_ */
