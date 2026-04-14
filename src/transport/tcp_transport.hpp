#pragma once

#include <string>
#include <arpa/inet.h>
#include <cstdint>
#include <utility> // for the use of pair
#include "itransport.hpp"

struct MessageHeader
{
    uint32_t procedure_id;
    uint32_t payload_length;
};

const uint8_t MAX_CONNECTTION_BACKLOG= 10;

class TCPTransport:public ITransport
{


    public:
        TCPTransport();
        ~TCPTransport();

        // move semantics
        TCPTransport(TCPTransport &&other) noexcept;
        TCPTransport &operator=(TCPTransport &&other) noexcept;

        // ensure the connection cannot be copied
        TCPTransport(const TCPTransport &) = delete;
        TCPTransport &operator=(const TCPTransport &) = delete;

        void bind(uint16_t port);
        void listen();
        TCPTransport accept();

        // client side 
        void connect(const std::string &host, uint16_t port);

        // both sides
        void send_message(uint32_t procedure_id, const std::string &payload) override;
        // <procedure_id,payload>
        std::pair<uint32_t, std::string> receive_message() override;
        std::string get_name() const override;

        void close();

    private:
        // since i am using stream sock, this is needed
        void send_exact(const void *data, size_t length); 
        void recv_exact(void *data, size_t length);

        int socket_fd = -1; // -1 if no socket
        std::string peer_name = "unknown";
};
