#include "placement.h"

namespace woops
{

void Placement::RegisterTable(const TableConfig& config) {
    configs_.push_back(config);
}

Placement::Partitions& Placement::GetPartitions(Tableid id) {
    return table_to_partitions_[id];
}

    
} /* woops */ 
