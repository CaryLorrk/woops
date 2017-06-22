#include <iostream>
#include <thread>
#include <algorithm>
#include "client/woops.h"
#include "client/woops_config.h"
#include "client/table_config.h"
#include "common/storage/storage.h"
#include "common/storage/dense_storage.h"
#include "common/logging.h"

constexpr int SIZE = 3;
int main()
{
    //woops::InitializeFromFile("/root/config.in");
    woops::InitializeFromFile("config.in");
    woops::TableConfig table_config;
    table_config.name = "test";
    table_config.size = SIZE;
    table_config.element_size = sizeof(float);
    table_config.server_storage_constructor = [](size_t size){
                     return std::unique_ptr<woops::Storage>(new woops::DenseStorage<float>(size));
                   
    };
    table_config.cache_constructor = [](size_t size){
                     return std::unique_ptr<woops::Storage>(new woops::DenseStorage<float>(size));
                   
    };
    woops::CreateTable(table_config);
    auto a = new float[SIZE];
    for (int i = 0; i < SIZE; ++i) {
        a[i] = i;
    }
    woops::LocalAssign("test", a);
    woops::ForceSync();
    std::fill(a, a+SIZE, 1);
    for(int i = 0; i < 10; ++i) {
        woops::Update("test", a);
        woops::Clock();
        woops::Sync("test");
    }
    delete[] a;
}
