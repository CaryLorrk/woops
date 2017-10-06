#include "context.h"

#include "common/logging.h"

namespace woops
{

Context& Context::Get() {
    static Context singleton;
    return singleton;
}

void Context::Initialize() {
    Context& ctx = Context::Get();
    ctx.this_host_ = 0;
    ctx.port_ = "50052";
    ctx.staleness_ = 0;
    ctx.hostnames_.push_back("127.0.0.1");
}

void Context::Initialize(const WoopsConfigProto& configproto) {
    Context& ctx = Context::Get();
    ctx.proto_init_hosts(configproto);
    ctx.port_ = configproto.port().empty() ? "50052" : configproto.port();
    ctx.staleness_ = configproto.staleness();

    LOG(INFO) << "hi, I'm host " << ctx.this_host_ << "(" << ctx.Hostname(ctx.ThisHost()) << ")";
    
    ctx.client_.Initialize();
    ctx.server_.Initialize();
    ctx.comm_.Initialize();
}

Comm& Context::Context::GetComm() {
    Context& ctx = Context::Get();
    return ctx.comm_;
}

Client& Context::Context::GetClient() {
    Context& ctx = Context::Get();
    return ctx.client_;
}

Server& Context::GetServer() {
    Context& ctx = Context::Get();
    return ctx.server_;
}

unsigned Context::ThisHost() {
    Context& ctx = Context::Get();
    return ctx.this_host_;
}

std::string Context::Hostname(unsigned host) {
    Context& ctx = Context::Get();
    return ctx.hostnames_[host];
}

size_t Context::NumHosts() {
    Context& ctx = Context::Get();
    return ctx.hostnames_.size();
}

std::string Context::Port() {
    Context& ctx = Context::Get();
    return ctx.port_;
}

unsigned Context::Staleness() {
    Context& ctx = Context::Get();
    return ctx.staleness_;
}


void Context::proto_init_hosts(const WoopsConfigProto& configproto) {
    if (configproto.hosts_size() == 0) {
        LOG(FATAL) << "configfile need hosts";
    }

    bool is_find_this_host = false;
    for (int host = 0; host < configproto.hosts_size(); ++host) {
        auto& hostname = configproto.hosts()[host];
        hostnames_.push_back(hostname);
        if (hostname == configproto.this_host()) {
            this_host_ = host;
            is_find_this_host = true;
        }
    }

    if (!is_find_this_host) {
        LOG(FATAL) << "cannot find this_host in configfile";
    }
}
    
} /* woops */ 
