#ifndef WOOPS_CLIENT_WOOPS_H_
#define WOOPS_CLIENT_WOOPS_H_

#include <string>

#include "common/protobuf/woops_config.pb.h"

#include "table.h"
#include "table_config.h"

namespace woops
{
void InitializeFromFile(const std::string& configfile);
void Initialize();
void Initialize(const WoopsConfigProto& configproto); 
void CreateTable(const TableConfig& config);
void LocalAssign(const std::string& name, const void* data);
void Update(const std::string& name, const void* data);
void Clock();
void Sync(const std::string& name);
void ForceSync();
std::string ToString();
} /* woops */ 

#endif /* end of include guard: WOOPS_CLIENT_WOOPS_H_ */
