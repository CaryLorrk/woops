#include "passive_ssp_consistency.h"

#include "lib.h"

namespace woops
{
PassiveSSPConsistency::PassiveSSPConsistency(Iteration staleness): staleness_(staleness) {}
void PassiveSSPConsistency::ClientSync(Tableid id, Iteration iteration) {
    auto&& table = Lib::Client()->GetTable(id);
    std::unique_lock<std::mutex> lock(table.mu);
    table.cv.wait(lock, [this, &table, iteration]{
        Iteration min = std::min_element(
                table.iterations.begin(), table.iterations.end(),
                [](ClientTable::Iterations::value_type& l,
                    ClientTable::Iterations::value_type& r) -> bool {
                    return l.second < r.second;
                })->second;
        return min >= iteration - staleness_ - 1;
    });

}    

void PassiveSSPConsistency::ClientUpdate(Tableid id,
        MAYBE_UNUSED const Storage& storage, Iteration iteration) {
    auto&& table = Lib::Client()->GetTable(id);
    // apply to gpu
    {
        std::unique_lock<std::mutex> lock(table.mu);
        if (table.need_apply) {
            table.worker_storage->Update(*table.apply_buffer);
            table.need_apply = false;
            table.apply_buffer->Zerofy();
        }
    }

    // push to server
    auto&& partitions = Lib::Placement()->GetPartitions(id);
    std::map<Hostid, Bytes> server_to_bytes;
    {
        std::lock_guard<std::mutex> lock(table.mu);
        server_to_bytes = table.transmit_buffer->Encode(partitions);
    }
    for (auto&& kv: server_to_bytes) {
        auto&& server = kv.first;
        auto&& bytes = kv.second;
        Lib::Comm()->ClientPush(server, id, iteration, std::move(bytes));
    }

    // pull from server
    Iteration min;
    {
        std::lock_guard<std::mutex> lock(table.mu);
        min = std::min_element(
                table.iterations.begin(), table.iterations.end(),
                [](ClientTable::Iterations::value_type& l,
                    ClientTable::Iterations::value_type& r) -> bool {
                    return l.second < r.second;
                })->second;
    }
    if (min < iteration - staleness_) {
        for (auto&& kv: partitions) {
            Lib::Comm()->ClientPull(kv.first, id, iteration);
        }
    }
}

void PassiveSSPConsistency::ServerPushHandler(Hostid server, Tableid id,
        Iteration iteration, const Bytes& bytes, Iteration iteration_now) {
    auto&& table = Lib::Client()->GetTable(id);
    table.cv.notify_all();
}

Iteration PassiveSSPConsistency::GetServerData(Hostid client, Tableid id, Iteration iteration) {
    auto&& table = Lib::Server()->GetTable(id);
    Iteration min;
    std::unique_lock<std::mutex> lock(table.mu); 
    table.cv.wait(lock, [this, &table, iteration, &min]{
        min = *std::min_element(table.iterations.begin(), table.iterations.end());
        return min >= iteration - staleness_;
    });
    return min;
}

void PassiveSSPConsistency::ClientPushHandler(Hostid client, Tableid id,
        Iteration iteration, const Bytes& bytes) {
    auto&& table = Lib::Server()->GetTable(id);
    Iteration min;
    {
        std::unique_lock<std::mutex> lock(table.mu); 
        min = *std::min_element(table.iterations.begin(), table.iterations.end());
    }
    if (min >= iteration - staleness_) {
        table.cv.notify_all();
    }

}
    
} /* woops */ 
