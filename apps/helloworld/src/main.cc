#include <iostream>
#include <thread>
#include <algorithm>
#include "woops.h"
#include "util/config/woops_config.h"
#include "util/config/table_config.h"
#include "util/storage/storage.h"
#include "util/storage/dense_storage.h"
#include "util/logging.h"

using StorageType = woops::DenseStorage<float>;

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
        table_config.server_storage_constructor = [](){
            return std::unique_ptr<woops::Storage>(new StorageType(SIZE));
                       
        };
        table_config.client_storage_constructor = [](){
            return std::unique_ptr<woops::Storage>(new StorageType(SIZE));
                       
        };
        table_config.apply_buffer_constructor = []() {
            return std::unique_ptr<woops::Storage>(new StorageType(SIZE));
        };
        woops::CreateTable(table_config);
    }
    std::vector<float> a(SIZE);
    for (int j = 0; j < NUM_TABLE; ++j) {
        for (int i = 0; i < SIZE; ++i) {
            a[i] = j*SIZE + i;
        }
        woops::LocalAssign(j, StorageType(a));
    }
    woops::Start();
    std::cout << woops::ToString() << std::endl;
    std::fill(a.begin(), a.end(), 1);
    for(int i = 0; i < MAX_ITER; ++i) {
        std::cout << "iteration: " << i << std::endl;
        for (int j = 0; j < NUM_TABLE; ++j) {
            woops::Sync(j);
            StorageType sa(a);
            woops::LocalUpdate(j, sa);
            woops::Update(j, sa);
        }
        woops::Clock();
    }
    for (int j = 0; j < NUM_TABLE; ++j) {
        woops::Sync(j);
    }
    std::cout << woops::ToString() << std::endl;
}
