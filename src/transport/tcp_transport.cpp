#include <string>
#include <iostream>
#include <utility>
#include <cerrno>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdexcept> // for standard exception classes
#include <cstring>
#include <sstream>
#include "tcp_transport.hpp"

//Note to self: documentation followed--> https://beej.us/guide/bgnet/pdf/bgnet_usl_c_1.pdf

TCPTransport :: TCPTransport(): socket_fd(-1){}

TCPTransport :: ~TCPTransport(){
    close();
};

//Move constructor
TCPTransport:: TCPTransport(TCPTransport &&other) noexcept: socket_fd(other.socket_fd), peer_name(std::move(other.peer_name)){
    other.socket_fd = -1;
    other.peer_name = "unknown";
};

TCPTransport& TCPTransport:: operator= (TCPTransport &&other) noexcept{
    if (this != &other){
        // first close the current connection
        close();
        socket_fd = other.socket_fd;
        peer_name = std::move(other.peer_name);
        other.socket_fd = -1;
        other.peer_name = "unknown";
    };
    return *this;
};

namespace {
std::runtime_error socket_error(const std::string& context) {
    std::ostringstream oss;
    oss << context << ": " << std::strerror(errno);
    return std::runtime_error(oss.str());
}

std::string endpoint_name(const sockaddr_in& addr) {
    char ip_buffer[INET_ADDRSTRLEN] = {0};
    if (::inet_ntop(AF_INET, &addr.sin_addr, ip_buffer, sizeof(ip_buffer)) == nullptr) {
        return "unknown";
    }

    return std::string(ip_buffer) + ":" + std::to_string(ntohs(addr.sin_port));
}
} // namespace

void TCPTransport::bind(uint16_t port){
    socket_fd = ::socket(AF_INET,SOCK_STREAM,0); // open a TCP socket with ipv4
    if(socket_fd <0){
        throw socket_error("Failed to create socket");
    };

    int opt = 1;
    // set socket options
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET; // using IPV4
    addr.sin_addr.s_addr = INADDR_ANY; // Allow the socket to bind to any IP address
    addr.sin_port        = htons(port);

    //https://pubs.opengroup.org/onlinepubs/009695099/functions/bind.html 
    if (::bind(socket_fd, (sockaddr*)&addr, sizeof(addr)) < 0){
        throw socket_error("Failed to bind to port " + std::to_string(port));
    }
    std::cout<<"Socket successfully bound to port "<<port<<std::endl;

}

void TCPTransport::close(){
    if (socket_fd>=0){
        ::close(socket_fd);
    }
    socket_fd = -1;
    peer_name = "unknown";

}

void TCPTransport::connect(const std::string &host, uint16_t port){
    socket_fd = socket(AF_INET,SOCK_STREAM,0);
    if(socket_fd<0){
        throw socket_error("Could not create client socket");
    };

    sockaddr_in addr{};
    addr.sin_family      = AF_INET; // using IPV4
    addr.sin_port        = htons(port);

    //https://man7.org/linux/man-pages/man3/inet_pton.3.html
    // attempt to convert the host string to a network address and store it in addr.sin_addr
    if(inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0){
        throw std::runtime_error("Could not initiate connection: invalid host provided (" + host + ")");
    }

    int result = ::connect(socket_fd, (sockaddr*)&addr,sizeof(addr));
    if (result <0){
        throw socket_error("Failed connecting to " + host + ":" + std::to_string(port));
    }
    peer_name = host + ":" + std::to_string(port);
    std::cout<<"Socket successfully connected: "<<host<<std::endl;
}

void TCPTransport::listen(){
if(::listen(socket_fd,MAX_CONNECTTION_BACKLOG)<0){
    throw socket_error("Failed to listen on socket");
}
}

TCPTransport TCPTransport::accept(){
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = ::accept(socket_fd, (sockaddr *) &client_addr, (socklen_t *)&addr_len);
    
    if (client_fd<0){
        throw socket_error("Failed to accept connection");
    };

    TCPTransport client;
    client.socket_fd = client_fd;
    client.peer_name = endpoint_name(client_addr);

    return client;
}

/**
 * `procedure_id`: the id of the procdure that made this call
 * `payload`: the call payload to send
 */
void TCPTransport::send_message(uint32_t procedure_id,const std::string &payload){
    MessageHeader header;
    header.procedure_id = htonl(procedure_id);
    header.payload_length = htonl(payload.size());

    send_exact(&header,sizeof(header));
    send_exact(payload.data(),payload.size()); //string.data() is the internal buffer
}

std::pair<uint32_t, std::string>  TCPTransport::receive_message(){
    MessageHeader header;
    recv_exact(&header,sizeof(header));

    int procedure_id = ntohl(header.procedure_id);
    int payload_length = ntohl(header.payload_length);
    std::string payload(payload_length, '\0');

    recv_exact(static_cast<void*>(&payload[0]), payload_length);

    return {procedure_id,payload};

}

std::string TCPTransport::get_name() const{
    return peer_name;
}

 void TCPTransport::send_exact(const void *data, size_t length){
    const char* ptr = static_cast<const char*>(data);
    size_t to_be_sent = length;

    while(to_be_sent >0){
        ssize_t sent = ::send(socket_fd,(void*)ptr,to_be_sent,0);
        if(sent <= 0){
            //The connection must have failed if we have more data left to send but sent is 0
            throw socket_error("Connection lost while sending data");
        }
        
        ptr += sent; // advance the pointer cursor for the next read
        to_be_sent -= sent;
    }

 }; 
void TCPTransport::recv_exact(void *data, size_t length){
    char* ptr = static_cast<char*>(data);
    size_t remainder = length;

    while(remainder>0){
        ssize_t received = ::recv(socket_fd, ptr, remainder, 0);
        if(received <= 0){
            //The connection must have failed if we have more data left to read but received is 0
            throw socket_error("Connection lost while reading data");
        }
        
        ptr += received; // advance the pointer cursor for the next read
        remainder -= received;
    }
};
