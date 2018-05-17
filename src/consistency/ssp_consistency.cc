#include "ssp_consistency.h"

#include "lib.h"

namespace woops
{
SSPConsistency::SSPConsistency(Iteration staleness): staleness_(staleness) {}
void SSPConsistency::ClientSync(Tableid id, Iteration iteration) {
    auto&& table = Lib::Client().GetTable(id);
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

void SSPConsistency::ClientUpdate(Tableid id,
        MAYBE_UNUSED const Storage& storage, 
        MAYBE_UNUSED Iteration iteration) {
    auto&& table = Lib::Client().GetTable(id);
    {
        std::unique_lock<std::mutex> lock(table.mu);
        if (table.need_apply) {
            table.worker_storage->Update(*table.apply_buffer);
            table.need_apply = false;
            table.apply_buffer->Zerofy();
        }
    }
}

void SSPConsistency::ServerPushHandler(MAYBE_UNUSED Hostid server, Tableid id,
        MAYBE_UNUSED Iteration iteration,
        MAYBE_UNUSED const Bytes& bytes,
        MAYBE_UNUSED Iteration iteration_now) {
    auto&& table = Lib::Client().GetTable(id);
    table.cv.notify_all();
}

Iteration SSPConsistency::GetServerData(MAYBE_UNUSED Hostid client,
        Tableid id, Iteration iteration) {
    auto&& table = Lib::Server().GetTable(id);
    Iteration min;
    std::unique_lock<std::mutex> lock(table.mu); 
    table.cv.wait(lock, [this, &table, iteration, &min]{
        min = *std::min_element(table.iterations.begin(), table.iterations.end());
        return min >= iteration - staleness_;
    });
    return min;
}

void SSPConsistency::ClientPushHandler(
        MAYBE_UNUSED Hostid client, Tableid id,
        Iteration iteration,
        MAYBE_UNUSED const Bytes& bytes) {
    auto&& table = Lib::Server().GetTable(id);
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
