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
    std::unique_ptr<Storage> storage;
    size_t size;
    size_t element_size;
    std::vector<int> iterations;
    std::mutex mu;
    std::condition_variable cv;


    unsigned assign_cnt;
}; 
} /* woops */ 

#endif /* end of include guard: WOOPS_SERVER_TABLE_H_ */
