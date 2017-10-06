#include <iostream>
#include <thread>
#include <algorithm>
#include "client/woops.h"
#include "client/table_config.h"
#include "common/storage/storage.h"
#include "common/storage/dense_storage.h"
#include "common/logging.h"

constexpr int SIZE = 100;
constexpr int NUM_TABLE = 30000;
int main()
{
    woops::InitializeFromFile("/root/config.in");
    //woops::InitializeFromFile("config.in");
    for (int i = 0; i < NUM_TABLE; ++i) {
        woops::TableConfig table_config;
        table_config.name = std::to_string(i);
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
        woops::LocalAssign(std::to_string(j), a);
    }
    woops::ForceSync();
    std::fill(a, a+SIZE, 1);
    for(int i = 0; i < 100; ++i) {
        std::cout << "iteration: " << i << std::endl;
        for (int j = 0; j < NUM_TABLE; ++j) {
            woops::Update(std::to_string(j), a);
        }
        woops::Clock();
        for (int j = 0; j < NUM_TABLE; ++j) {
            woops::Sync(std::to_string(j));
        }
    }
    delete[] a;
    std::cout << woops::ToString() << std::endl;
}
