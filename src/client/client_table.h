#ifndef WOOPS_CLIENT_TABLE_H_
#define WOOPS_CLIENT_TABLE_H_

#include <memory>
#include <mutex>
#include <condition_variable>

#include "util/storage/storage.h"
#include "util/config/table_config.h"

namespace woops
{
struct ClientTable {
    TableConfig config;
    size_t size;
    size_t element_size;

    std::mutex  mu;
    std::condition_variable cv;

    using Iterations = std::map<Hostid, Iteration>;
    std::unique_ptr<Storage> worker_storage; 
    Iterations iterations;

    std::unique_ptr<Storage> transmit_buffer;
    std::unique_ptr<Storage> apply_buffer;
    bool need_apply;
};
} /* woops */ 



#endif /* end of include guard: WOOPS_CLIENT_TABLE_H_ */
