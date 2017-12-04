#include "lib.h"

#include <fstream>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "util/protobuf/woops_config.pb.h"
#include "util/logging.h"

using google::protobuf::io::IstreamInputStream;
using google::protobuf::TextFormat;

namespace woops
{
Lib& Lib::Get() {
    static Lib lib;
    return lib;
}

void Lib::Initialize(const WoopsConfig& config) {
    Lib& lib = Get();
    lib.config_ = config;

    lib.comm_.Initialize(config, &lib.client_, &lib.server_);
    lib.client_.Initialize(config, &lib.comm_);
    lib.server_.Initialize(config, &lib.comm_);
}

void Lib::InitializeFromFile(const std::string& filename) {
    WoopsConfigProto configproto;
    std::ifstream ifs(filename);
    if (!ifs) {
        LOG(FATAL) << "Configure file opening failed.";
    }
    IstreamInputStream iis(&ifs);
    TextFormat::Parse(&iis, &configproto);

    WoopsConfig config(configproto);
    Initialize(config);
}

void Lib::CreateTable(const TableConfig& config) {
    Lib& lib = Get();
    lib.client_.CreateTable(config);
}

void Lib::LocalAssign(const std::string& tablename, const void* data) {
    Lib& lib = Get();
    lib.client_.LocalAssign(tablename, data);
}

void Lib::Update(const std::string& tablename, const void* data) {
    Lib& lib = Get();
    lib.client_.Update(tablename, data);
}

void Lib::Clock() {
    Lib& lib = Get();
    lib.client_.Clock();
}

void Lib::Sync(const std::string& tablename) {
    Lib& lib = Get();
    lib.client_.Sync(tablename); 
}

void Lib::ForceSync() {
    Lib& lib = Get();
    lib.client_.ForceSync();
}

std::string Lib::ToString() {
    Lib& lib = Get();
    return lib.client_.ToString();
}
} /* woops */ 
