#include "traditional_ssp_consistency.h"

#include "lib.h"

namespace woops
{

TraditionalSSPConsistency::TraditionalSSPConsistency(Iteration staleness): SSPConsistency(staleness) {}

void TraditionalSSPConsistency::BeforeClock(Iteration iteration) {

    auto&& tables = Lib::Client().GetTables();
    for (auto&& kv: tables) {
        Tableid id = kv.first;
        auto&& table = *kv.second;
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
    }
}
    
} /* woops */ 
