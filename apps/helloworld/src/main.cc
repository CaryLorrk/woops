#include <iostream>
#include <thread>
#include <algorithm>
#include "woops.h"
#include "util/config/woops_config.h"
#include "util/config/table_config.h"
#include "util/storage/storage.h"
#include "util/storage/dense_storage.h"
#include "util/logging.h"

constexpr int SIZE = 3;
constexpr int NUM_TABLE = 3;
constexpr int MAX_ITER = 5;
int main()
{
    woops::InitializeFromFile("/root/config.in");
    //woops::InitializeFromFile("config.in");
    for (int i = 0; i < NUM_TABLE; ++i) {
        woops::TableConfig table_config;
        table_config.id = i;
        table_config.size = SIZE;
        table_config.element_size = sizeof(float);
        table_config.server_storage_constructor = [](size_t size){
                         return std::unique_ptr<woops::Storage>(new woops::DenseStorage<float>(size));
                       
        };
        table_config.cache_constructor = [](size_t size){
                         return std::unique_ptr<woops::Storage>(new woops::DenseStorage<float>(size));
                       
        };
        woops::CreateTable(table_config);
    }
    auto a = new float[SIZE];
    for (int j = 0; j < NUM_TABLE; ++j) {
        for (int i = 0; i < SIZE; ++i) {
            a[i] = j*SIZE + i;
        }
        woops::LocalAssign(j, a);
    }
    woops::ForceSync();
    std::fill(a, a+SIZE, 1);
    for(int i = 0; i < MAX_ITER; ++i) {
        std::cout << "iteration: " << i << std::endl;
        for (int j = 0; j < NUM_TABLE; ++j) {
            woops::Sync(j);
            woops::DenseStorage<float> sa(SIZE);
            sa.Assign(a);
            woops::Update(j, sa);
        }
        woops::Clock();
    }
    delete[] a;
    for (int j = 0; j < NUM_TABLE; ++j) {
        woops::Sync(j);
    }
    std::cout << woops::ToString() << std::endl;
}
