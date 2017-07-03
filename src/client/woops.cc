#include "woops.h"

#include <fstream>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "common/protobuf/woops_config.pb.h"
#include "common/logging.h"
#include "client.h"

using google::protobuf::io::IstreamInputStream;
using google::protobuf::TextFormat;

namespace woops
{

void InitializeFromFile(const std::string& configfile) {
    WoopsConfigProto config_proto;
    std::ifstream ifs(configfile);
    if (!ifs) {
        LOG(FATAL) << "Configure file opening failed.";
    }
    IstreamInputStream iis(&ifs);
    TextFormat::Parse(&iis, &config_proto);

    WoopsConfig config(config_proto);
    Initialize(config);
}

void Initialize(const WoopsConfig& config){
    Client::GetInstance().Initialize(config);
}

void CreateTable(const TableConfig& config){
    return Client::GetInstance().CreateTable(config);
}

void LocalAssign(const std::string& name, const void* data) {
    Client::GetInstance().LocalAssign(name, data); 
}

void Update(const std::string& name, const void* data){
    Client::GetInstance().Update(name, data); 
}

void Clock() {
    Client::GetInstance().Clock();
}

void Sync(const std::string& name) {
    Client::GetInstance().Sync(name);
}

void ForceSync() {
    Client::GetInstance().ForceSync();
}

std::string ToString() {
    return Client::GetInstance().ToString();
}
    
} /* woops */ 
