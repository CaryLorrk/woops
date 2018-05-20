#include "aggressive_ssp_consistency.h"

#include "lib.h"

namespace woops
{

AggressiveSSPConsistency::AggressiveSSPConsistency(Iteration staleness): SSPConsistency(staleness){}


void AggressiveSSPConsistency::ClientUpdate(Tableid id,
        MAYBE_UNUSED const Storage& storage, Iteration iteration) {
    auto&& table = Lib::Client().GetTable(id);
    // apply to gpu
    SSPConsistency::ClientUpdate(id, storage, iteration);

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

} /* woops */ 
