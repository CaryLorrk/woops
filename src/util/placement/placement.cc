#include "placement.h"

namespace woops
{

void Placement::RegisterTable(const TableConfig& config) {
    configs_.push_back(config);
}

Placement::Partitions& Placement::GetPartitions(Tableid id) {
    return table_to_partitions_[id];
}

std::string Placement::ToString() {
    std::stringstream ss;
    for (auto& kv: table_to_partitions_) {
        Tableid tableid = kv.first;
        Partitions& partitions = kv.second;
        ss << "table: " << tableid << '\n';
        for (auto& kv: partitions) {
            Hostid hostid = kv.first;
            Partition& partition = kv.second;
            ss << hostid << " " << partition.begin << " " << partition.end << '\n';
        }
        
    }
    return ss.str();
}

template<typename T>
void str_bytes_append(std::string& ret, T data) {
    ret.append((char*)&data, sizeof(data));
}

std::string Placement::Serialize() {
    std::string ret;
    for(auto& kv: table_to_partitions_) {
        Tableid tableid = kv.first;
        Partitions partitions = kv.second;
        str_bytes_append(ret, tableid);
        for(auto& kv: partitions) {
            Hostid server = kv.first;
            Partition partition = kv.second;
            str_bytes_append(ret, server);
            str_bytes_append(ret, partition.begin);
            str_bytes_append(ret, partition.end);
        }
        str_bytes_append(ret, (Hostid)-1);
    }
    return ret;
}

template<typename T>
void str_decode(const char** pp, T& ret) {
    ret = *((T*)(*pp));
    *pp = *pp + sizeof(T);
}

void Placement::Deserialize(const std::string& data) {
    table_to_partitions_.clear();
    const char* p = data.data();
    while (p < data.data()+data.size()) {
        Tableid tableid;
        str_decode(&p, tableid);
        Hostid server;
        Partitions partitions;
        Partition partition;
        while (true) {
            str_decode(&p, server);
            if (server == -1) break;
            str_decode(&p, partition.begin);
            str_decode(&p, partition.end);
            partitions[server] = partition;
        }
        table_to_partitions_[tableid] = partitions;
    }
}

    
} /* woops */ 
