#include "woops.h"

#include <fstream>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "common/logging.h"
#include "common/context.h"

using google::protobuf::io::IstreamInputStream;
using google::protobuf::TextFormat;

namespace woops
{

void InitializeFromFile(const std::string& configfile) {
    WoopsConfigProto configproto;

    std::ifstream ifs(configfile);
    if (!ifs) {
        LOG(FATAL) << "Configure file opening failed.";
    }
    IstreamInputStream iis(&ifs);
    TextFormat::Parse(&iis, &configproto);

    Initialize(configproto);
}

void Initialize() {
    Context::Initialize();
}

void Initialize(const WoopsConfigProto& configproto){
    Context::Initialize(configproto);
}

void CreateTable(const TableConfig& config){
    return Context::GetClient().CreateTable(config);
}

void LocalAssign(const std::string& name, const void* data) {
    Context::GetClient().LocalAssign(name, data); 
}

void Update(const std::string& name, const void* data){
    Context::GetClient().Update(name, data); 
}

void Clock() {
    Context::GetClient().Clock();
}

void Sync(const std::string& name) {
    Context::GetClient().Sync(name);
}

void ForceSync() {
    Context::GetClient().ForceSync();
}

std::string ToString() {
    return Context::GetClient().ToString();
}
    
} /* woops */ 
