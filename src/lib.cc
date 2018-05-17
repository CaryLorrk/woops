#include "lib.h"

#include <fstream>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "util/protobuf/woops_config.pb.h"
#include "util/logging.h"

#include "util/placement/placement.h"
#include "util/comm/comm.h"
#include "client/client.h"
#include "server/server.h"

#include "util/placement/uniform_split_placement.h"
#include "util/placement/round_robin_placement.h"
#include "util/placement/greedy_placement.h"

#include "consistency/passive_ssp_consistency.h"
#include "consistency/aggressive_ssp_consistency.h"
#include "consistency/adaptive_consistency.h"

using google::protobuf::io::IstreamInputStream;
using google::protobuf::TextFormat;

namespace woops
{
void Lib::Initialize(const WoopsConfig& config) {
    Lib& lib = Get();
    lib.woops_config_ = config;
    if (config.consistency == "passive") {
        lib.consistency_ = std::make_unique<PassiveSSPConsistency>(config.staleness);
    } else if (config.consistency == "aggressive") {
        lib.consistency_ = std::make_unique<AggressiveSSPConsistency>(config.staleness);
    } else if (config.consistency == "adaptive") {
        lib.consistency_ = std::make_unique<AdaptiveConsistency>(config.staleness);
    } else {
        LOG(FATAL) << "Unknown consistency: " << config.consistency;
    }

    if (config.placement == "rr") {
        lib.placement_ = std::make_unique<RoundRobinPlacement>();
    } else if (config.placement == "greedy") {
        lib.placement_ = std::make_unique<GreedyPlacement>();
    } else if (config.placement == "uniform") {
        lib.placement_ = std::make_unique<UniformSplitPlacement>();
    } else {
        LOG(FATAL) << "Unknown placement: " << config.placement;
    }
    lib.client_ = std::make_unique<woops::Client>();
    lib.server_ = std::make_unique<woops::Server>();
    lib.comm_ = std::make_unique<woops::Comm>();
    lib.comm_->Initialize();
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
    lib.table_configs_.push_back(config);
    lib.client_->CreateTable(config);
}

void Lib::LocalAssign(Tableid id, const Storage& data) {
    Lib& lib = Get();
    lib.client_->LocalAssign(id, data);
}

void Lib::Update(Tableid id, const Storage& data) {
    Lib& lib = Get();
    lib.client_->Update(id, data);
}

void Lib::Clock() {
    Lib& lib = Get();
    lib.client_->Clock();
}

void Lib::Sync(Tableid id) {
    Lib& lib = Get();
    lib.client_->Sync(id); 
}

void Lib::Start() {
    Lib& lib = Get();
    lib.client_->Start();
}

std::string Lib::ToString() {
    Lib& lib = Get();
    return lib.client_->ToString();
}

} /* woops */ 
