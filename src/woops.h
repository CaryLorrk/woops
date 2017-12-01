#ifndef WOOPS_WOOPS_H_
#define WOOPS_WOOPS_H_

#include <string>

#include "util/config/table_config.h"
#include "util/config/woops_config.h"

namespace woops
{
void Initialize(const WoopsConfig& config); 
void InitializeFromFile(const std::string& filename);
void CreateTable(const TableConfig& config);
void LocalAssign(const std::string& tablename, const void* data);
void Update(const std::string& tablename, const void* data);
void Clock();
void Sync(const std::string& tablename);
void ForceSync();
std::string ToString();
} /* woops */ 

#endif /* end of include guard: WOOPS_WOOPS_H_ */
