#include <iostream>
#include <thread>
#include <algorithm>
#include "woops.h"
#include "util/config/woops_config.h"
#include "util/config/table_config.h"
#include "util/storage/storage.h"
#include "util/storage/dense_storage.h"
#include "util/logging.h"

#include "storage/client_storage.h"
#include "storage/transmit_buffer.h"
#include "storage/server_storage.h"
#include "storage/apply_buffer.h"

using Gradient = woops::DenseStorage<float>;

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
        table_config.client_storage_constructor = []() -> woops::Storage*{
            return new woops::ClientStorage<float>(SIZE);
                       
        };
        table_config.transmit_buffer_constructor = []() -> woops::Storage* {
            return new woops::TransmitBuffer<float>(SIZE);
        };
        table_config.server_storage_constructor = []() -> woops::Storage*{
            return new woops::ServerStorage<float>(SIZE);
        };
        table_config.apply_buffer_constructor = []() -> woops::Storage*{
            return new woops::ApplyBuffer<float>(SIZE);
        };
        woops::CreateTable(table_config);
    }
    std::vector<float> a(SIZE);
    for (int j = 0; j < NUM_TABLE; ++j) {
        for (int i = 0; i < SIZE; ++i) {
            a[i] = j*SIZE + i;
        }
        woops::LocalAssign(j, Gradient(a));
    }
    woops::Start();
    std::cout << woops::ToString() << std::endl;
    std::fill(a.begin(), a.end(), 1);
    for(int i = 0; i < MAX_ITER; ++i) {
        std::cout << "iteration: " << i << std::endl;
        for (int j = 0; j < NUM_TABLE; ++j) {
            woops::Sync(j);
            woops::Update(j, Gradient(a));
        }
        woops::Clock();
    }
    for (int j = 0; j < NUM_TABLE; ++j) {
        woops::Sync(j);
    }
    std::cout << woops::ToString() << std::endl;
}
