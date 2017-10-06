#include "comm.h"

#include <thread>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include "context.h"
#include "logging.h"

using namespace std::chrono_literals;
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::ArrayInputStream;
using google::protobuf::io::CodedOutputStream;
using google::protobuf::io::ArrayOutputStream;

namespace woops
{

void Comm::Initialize() {
    sockfds_.resize(Context::NumHosts());
    init_ip_to_host_();
    std::thread server_for_connections_thread(&Comm::server_for_connections_func_, this);
    client_for_connections_();
    server_for_connections_thread.join();
    pull_thread_ = std::thread(&Comm::pull_func_, this);
    std::this_thread::sleep_for(10ms);
}

void Comm::RegisterMessageHandler(rpc::Message_Command cmd, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(msg_handler_mu_);
    msg_handler_[cmd] = handler;
}

void Comm::SendMessage(unsigned host, const rpc::Message& msg) {
    static std::mutex mu_;
    //LOG(INFO) << "SendMessage host: " << host << " cmd: " << msg.Command_Name(msg.cmd());
    size_t msg_size = msg.ByteSizeLong();
    size_t pkt_size = msg_size + 4;
    std::vector<int8_t> pkt(pkt_size);
    ArrayOutputStream array_output_stream(pkt.data(), pkt_size);
    CodedOutputStream coded_output_stream(&array_output_stream);
    coded_output_stream.WriteVarint32(msg_size);
    msg.SerializeToCodedStream(&coded_output_stream);
    int numbytes;
    {
    std::lock_guard<std::mutex> lock(mu_);
    numbytes = send(sockfds_[host], pkt.data(), pkt_size, 0);

    }
    //if (msg.cmd() == rpc::Message_Command_UPDATE) {
        //auto& update = msg.update();
        //LOG(INFO) << "SendMessage Update host: " << host << ", table: " << update.tablename() << ", iter: " << update.iteration() << ", msg_size: " << msg_size << ", pkt_size: " << pkt_size << ", numbytes: " << numbytes;
    //}
    if (numbytes < 0) {
        LOG(ERROR);
        perror("send");
        exit(1);
    } else if (numbytes < pkt_size){
        LOG(FATAL) << "numbytes: " << numbytes << ", pkt_size: " << pkt_size;
    }
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void Comm::init_ip_to_host_() {
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *hostinfo;
    addrinfo *p;
    int rv;
    char ip[INET6_ADDRSTRLEN];
    for (unsigned host = 0; host < Context::NumHosts(); ++host) {
        if ((rv = getaddrinfo(Context::Hostname(host).c_str(),
                        Context::Port().c_str(), &hints, &hostinfo)) != 0) {
            LOG(FATAL) << "getaddrinfo: " << gai_strerror(rv);
        }

        for(p = hostinfo; p != NULL; p = p->ai_next) {
            inet_ntop(p->ai_family,  get_in_addr(p->ai_addr), ip, sizeof(ip));
            ip_to_host_[ip] = host; 
        }
        freeaddrinfo(hostinfo);
    }
}


int bind_for_connections() {
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo *serverinfo;
    int rv;
	if ((rv = getaddrinfo(NULL, Context::Port().c_str(), &hints, &serverinfo)) != 0) {
        LOG(FATAL) << "getaddrinfo: " << gai_strerror(rv);
	}

    int sockfd;
    addrinfo *p;
	for(p = serverinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
            LOG(ERROR);
			perror("socket");
			continue;
		}

        int yes = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
            LOG(ERROR);
			perror("setsockopt");
            exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
            LOG(ERROR);
			perror("bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		LOG(FATAL) << "failed to bind";
	}

	freeaddrinfo(serverinfo); // all done with this structure
    return sockfd;
}

void listen_for_connections(int server_sockfd) {
	if (listen(server_sockfd, Context::NumHosts()) == -1) {
        LOG(ERROR);
		perror("listen");
		exit(1);
	}
}

void Comm::accept_for_connections_(int server_sockfd) {
    int client_sockfd;
	socklen_t sin_size;
    sockaddr_storage their_addr;
    char ip[INET6_ADDRSTRLEN];
    for(unsigned cnt = 0; cnt < Context::ThisHost(); ++cnt) {
		sin_size = sizeof(their_addr);
		client_sockfd = accept(server_sockfd,
                (struct sockaddr *)&their_addr, &sin_size);
		if (client_sockfd == -1) {
            LOG(ERROR);
			perror("accept");
            exit(1);
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			ip, sizeof(ip));
        unsigned host = ip_to_host_[ip];
        sockfds_[host] = client_sockfd;
        sockfd_to_host_[client_sockfd] = host;

		LOG(INFO) << "got connection from " << ip;
        
    }
}


void Comm::server_for_connections_func_() {
    int server_sockfd = bind_for_connections();
    listen_for_connections(server_sockfd);
	LOG(INFO) << "waiting for connections...";
    accept_for_connections_(server_sockfd);
    close(server_sockfd);
}

void Comm::client_for_connections_() {
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int sockfd;
    int rv;
    addrinfo *serverinfo;
    addrinfo *p;
    for (unsigned host = Context::ThisHost() + 1; host < Context::NumHosts(); ++host) {
        std::string hostname = Context::Hostname(host);
        if ((rv = getaddrinfo(hostname.c_str(),
                        Context::Port().c_str(), &hints, &serverinfo)) != 0) {
            LOG(FATAL) << "getaddrinfo: " << gai_strerror(rv);
        }

		int yes=1;
        while (true) {
            for(p = serverinfo; p != NULL; p = p->ai_next) {
                if ((sockfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1) {
                    LOG(ERROR);
                    perror("socket");
                    continue;
                }
				if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
						sizeof(int)) == -1) {
                    LOG(ERROR);
					perror("setsockopt");
					exit(1);
				}

                if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                    LOG(ERROR);
                    perror("connect");
                    close(sockfd);
                    continue;
                }
                break;
            }
            if (p == NULL) {
                LOG(WARNING) << "failed to connect to " << hostname;
                std::this_thread::sleep_for(1s);
                
            } else {
                LOG(INFO) << "connect to " << hostname;
                break;
            }
        }
        sockfds_[host] = sockfd;
        sockfd_to_host_[sockfd] = host;
        freeaddrinfo(serverinfo);
    }
}

size_t Comm::read_msg_size_(int sockfd)
{
    int8_t buffer[4];
    memset(buffer, '\0', 4);
    int numbytes = recv(sockfd, buffer, 4, MSG_PEEK);
    if (numbytes < 0) {
        LOG(ERROR);
        perror("recv");
        exit(1);
    } else if (numbytes == 0) {
        LOG(INFO) << Context::Hostname(sockfd_to_host_[sockfd]) << " disconncted";
        exit(0);
    } else if (numbytes < 4) {
        LOG(FATAL) << "numbytes: " << numbytes;
    }
    google::protobuf::uint32 size;
    ArrayInputStream array_input_stream(buffer, 4);
    CodedInputStream coded_input_stream(&array_input_stream);
    coded_input_stream.ReadVarint32(&size);
    return size;
}

void Comm::parse_msg_dispatch_(unsigned host, std::vector<int8_t> buffer, size_t pkt_size, int numbytes) {
    ArrayInputStream array_input_stream(buffer.data(), pkt_size);
    CodedInputStream coded_input_stream(&array_input_stream);

    google::protobuf::uint32 msg_size;
    coded_input_stream.ReadVarint32(&msg_size);

    CodedInputStream::Limit msg_limit = coded_input_stream.PushLimit(msg_size);
    rpc::Message msg;
    msg.ParseFromCodedStream(&coded_input_stream);
    coded_input_stream.PopLimit(msg_limit);
    
    //LOG(INFO) << "recv from host: " << host << " command: " << msg.Command_Name(msg.cmd());
    //if (msg.cmd() == rpc::Message_Command_UPDATE) {
        //auto& update = msg.update();
        //auto& table = update.tablename();
        //int iter  = update.iteration();
            //LOG(INFO) << "table: " << table << ", iter: " << iter << ", msg_size: " << msg_size << ", pkt_size: " << pkt_size << ", numbytes: " << numbytes;
    //}
    auto search = msg_handler_.find(msg.cmd());
    if (search == msg_handler_.end()) {
        LOG(FATAL) << "unknown command";
    }
    msg_handler_[msg.cmd()](host, msg);
}

void Comm::read_msg_body_(int sockfd)
{
    unsigned host = sockfd_to_host_[sockfd];
    size_t msg_size = read_msg_size_(sockfd);
    size_t pkt_size = msg_size + 4;
    std::vector<int8_t> buffer(pkt_size);

    int numbytes = recv(sockfd, buffer.data(), pkt_size, MSG_WAITALL);
    if(numbytes < 0){
        LOG(ERROR);
        perror("recv");
        exit(1);
    } else if (numbytes == 0) {
        LOG(INFO) << Context::Hostname(host) << " disconncted";
        exit(0);
    } else if (numbytes < pkt_size) {
        LOG(FATAL) << "numbytes: " << numbytes << ", pkt_size: " << pkt_size;
    }

    std::thread(&Comm::parse_msg_dispatch_, this, host, std::move(buffer), pkt_size, numbytes).detach();
}

void Comm::set_host_events(int epollfd, std::vector<epoll_event>& host_events) {
    int rv;
    for (unsigned host = 0; host < Context::NumHosts(); ++host) {
        if (host == Context::ThisHost()) continue;

        int sockfd = sockfds_[host];
        epoll_event& ev = host_events[host];
        ev.data.fd = sockfd;
        ev.events = EPOLLIN;
        if ((rv = epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev)) < 0) {
            LOG(ERROR);
            perror("epoll_ctl");
            exit(1);
        }
    }
}

void Comm::pull_func_() {
    int epollfd = epoll_create1(0);
    if (epollfd < 0) {
        LOG(ERROR);
        perror("epoll_create1");
        exit(1);
    }

    std::vector<epoll_event> host_events(Context::NumHosts());
    set_host_events(epollfd, host_events);

    int maxevent = Context::NumHosts();
    std::vector<epoll_event> avail_events(maxevent);
    while(true) {
        int nfds = epoll_wait(epollfd, avail_events.data(), maxevent, -1);
        if (nfds < 0) {
            LOG(ERROR);
            perror("epoll_wait");
            exit(1);
        }

        for (int n = 0; n < nfds; ++n) {
            int sockfd = avail_events[n].data.fd;
            read_msg_body_(sockfd);
        }
    }
}

} /* woops */ 
