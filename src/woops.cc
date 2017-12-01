#include "woops.h"

#include "lib.h"

namespace woops
{

void Initialize(const WoopsConfig& config){
    Lib::Initialize(config);
}
void InitializeFromFile(const std::string& filename) {
    Lib::InitializeFromFile(filename);
}

void CreateTable(const TableConfig& config){
    Lib::CreateTable(config);
}

void LocalAssign(const std::string& tablename, const void* data) {
    Lib::LocalAssign(tablename, data);
}

void Update(const std::string& tablename, const void* data){
    Lib::Update(tablename, data);
}

void Clock() {
    Lib::Clock();
}

void Sync(const std::string& tablename) {
    Lib::Sync(tablename);
}

void ForceSync() {
    Lib::ForceSync();
}

std::string ToString() {
    return Lib::ToString();
}
    
} /* woops */ 
