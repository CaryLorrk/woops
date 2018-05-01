#ifndef WOOPS_PLACEMENT_PLACEMENT_H_
#define WOOPS_PLACEMENT_PLACEMENT_H_

#include <map>
#include <utility>

#include "util/typedef.h"
#include "util/config/table_config.h"
#include "util/config/woops_config.h"

namespace woops
{
class Placement
{
public:
    struct Partition {
        ParamIndex begin;
        ParamIndex end;
    };
    using Partitions = std::map<Hostid, Partition>;

    virtual void Decision() = 0;

    const Partitions& GetPartitions(Tableid id);
    std::string Serialize();
    void Deserialize(const std::string& data);
    std::string ToString();
protected:
    std::map<Tableid, Partitions> table_to_partitions_;

    template<typename T>
    void str_bytes_append(std::string& ret, T data);

    template<typename T>
    void str_decode(const char** pp, T& ret);

};
} /* woops */ 

#endif /* end of include guard: WOOPS_PLACEMENT_PLACEMENT_H_ */
