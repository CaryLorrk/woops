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

void LocalAssign(int id, const void* data) {
    Lib::LocalAssign(id, data);
}

void Update(int id, const void* data){
    Lib::Update(id, data);
}

void Clock() {
    Lib::Clock();
}

void Sync(int id) {
    Lib::Sync(id);
}

void Start() {
    Lib::Start();
}

std::string ToString() {
    return Lib::ToString();
}
    
} /* woops */ 
