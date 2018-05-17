#ifndef WOOPS_SERVER_TABLE_H_
#define WOOPS_SERVER_TABLE_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>
#include <atomic>

#include "util/storage/storage.h"

namespace woops
{
struct ServerTable {
    size_t size;
    size_t element_size;

    std::mutex mu;
    std::condition_variable cv;

    std::vector<std::unique_ptr<Storage>> storages;
    std::vector<Iteration> iterations;
}; 
} /* woops */ 

#endif /* end of include guard: WOOPS_SERVER_TABLE_H_ */
