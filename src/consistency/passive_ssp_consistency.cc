#include "passive_ssp_consistency.h"

#include "lib.h"

namespace woops
{
PassiveSSPConsistency::PassiveSSPConsistency(Iteration staleness): SSPConsistency(staleness) {}

void PassiveSSPConsistency::AfterClientUpdate(Tableid id,
        MAYBE_UNUSED const Storage& storage, Iteration iteration) {

    auto&& table = Lib::Client().GetTable(id);
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
        // push to server
        auto&& partitions = Lib::Placement().GetPartitions(id);
        std::map<Hostid, Bytes> server_to_bytes;
        {
            std::lock_guard<std::mutex> lock(table.mu);
            server_to_bytes = table.transmit_buffer->Encode(partitions);
        }
        for (auto&& kv: partitions) {
            auto&& server = kv.first;
            auto&& bytes = server_to_bytes[server];
            Lib::Comm().ClientPush(server, id, iteration, std::move(bytes));
        }

        // pull from server
        for (auto&& kv: partitions) {
            Lib::Comm().ClientPull(kv.first, id, iteration);
        }
    }

    // apply to gpu
    SSPConsistency::AfterClientUpdate(id, storage, iteration);
}

    
} /* woops */ 
