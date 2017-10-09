#ifndef WOOPS_COMMON_THREAD_POOL_H_
#define WOOPS_COMMON_THREAD_POOL_H_

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace woops
{
class ThreadPool
{
public:
    enum Priority: int {
        NONE = -1,
        HIGH = 0,
        MEDIAN = 1,
        LOW = 2
    };
    ThreadPool (size_t size);
    ~ThreadPool();
    void Push(Priority pr, std::function<void()> f);

private:
    std::vector<std::thread> workers_;
    std::vector<std::queue<std::function<void()>>> task_queues_;

    std::mutex mu_;
    std::condition_variable cv_;
    bool end_;

    Priority check_queues();
};
    
} /* woops */ 



#endif /* end of include guard: WOOPS_COMMON_THREAD_POOL_H_ */
