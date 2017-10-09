#ifndef WOOPS_COMMON_COMM_H_
#define WOOPS_COMMON_COMM_H_

#include <vector>
#include <map>
#include <thread>
#include <functional>
#include <mutex>

#include <sys/epoll.h>

#include "common/protobuf/rpc_message.pb.h"

namespace woops
{
class Comm
{
public:
    using MessageHandler = std::function<void(unsigned, const rpc::Message&)>;
    void Initialize();
    void RegisterMessageHandler(rpc::Message_Command cmd, MessageHandler handler);
    void SendMessage(unsigned host, const rpc::Message& msg);

private:
    std::vector<unsigned> sockfds_; 
    std::map<std::string, unsigned> ip_to_host_;
    std::map<int, unsigned> sockfd_to_host_;

    std::mutex msg_handler_mu_;
    std::map<rpc::Message_Command, MessageHandler> msg_handler_;

    std::thread pull_thread_;

    void init_ip_to_host_();
    void server_for_connections_func_();
    void accept_for_connections_(int server_sockfd);
    void client_for_connections_();
    void pull_func_();
    size_t read_msg_size_(int sockfd);
    void read_msg_body_(int sockfd);
    void parse_msg_dispatch_(unsigned host, std::vector<int8_t> buffer, size_t pkt_size);
    void set_host_events(int epollfd, std::vector<epoll_event>& host_events);


};
    
} /* woops */ 



#endif /* end of include guard: WOOPS_COMMON_COMM_H_ */
