#ifndef WOOPS_CLIENT_WOOPS_H_
#define WOOPS_CLIENT_WOOPS_H_

#include <string>

#include "table.h"
#include "woops_config.h"
#include "table_config.h"

namespace woops
{
void InitializeFromFile(const std::string& configfile);
void Initialize(const WoopsConfig& config); 
void CreateTable(const TableConfig& config);
void LocalAssign(const std::string& name, const void* data);
void Update(const std::string& name, const void* data);
void Clock();
void Sync(const std::string& name);
void ForceSync();
} /* woops */ 

#endif /* end of include guard: WOOPS_CLIENT_WOOPS_H_ */
