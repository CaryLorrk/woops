#include "thread_pool.h"

namespace woops
{

ThreadPool::ThreadPool(size_t size):
    task_queues_(3),
    end_(false)
{
    for(size_t i = 0; i < size; ++i) {
        workers_.emplace_back([this]{
            while(true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mu_);
                    this->cv_.wait(lock, [this]{
                        return end_ || check_queues() != Priority::NONE;
                    });
                    Priority pr = check_queues();
                    if(end_ && pr == Priority::NONE) return;
                    task = std::move(task_queues_[pr].front());
                    task_queues_[pr].pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mu_);
        end_ = true;
    }
    cv_.notify_all();
    for(auto& worker: workers_) {
        worker.join();
    }
}

void ThreadPool::Push(Priority pr, std::function<void()> f) {
    {
        std::unique_lock<std::mutex> lock(mu_);    
        task_queues_[pr].emplace(std::move(f));
    }
    cv_.notify_one();
}

ThreadPool::Priority ThreadPool::check_queues() {
    if(!task_queues_[Priority::HIGH].empty()) {
        return Priority::HIGH;
    }
    if(!task_queues_[Priority::MEDIAN].empty()) {
        return Priority::MEDIAN;
    } 
    if(!task_queues_[Priority::LOW].empty()) {
        return Priority::LOW;
    }
    return Priority::NONE;
}

    
} /* woops */ 
