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

void LocalAssign(Tableid id, const void* data) {
    Lib::LocalAssign(id, data);
}

void Update(Tableid id, Storage& data){
    Lib::Update(id, data);
}

void Clock() {
    Lib::Clock();
}

void Sync(Tableid id) {
    Lib::Sync(id);
}

void ForceSync() {
    Lib::ForceSync();
}

std::string ToString() {
    return Lib::ToString();
}
    
} /* woops */ 
