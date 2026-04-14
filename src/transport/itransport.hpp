#pragma once

#include <string>
#include <arpa/inet.h>
#include <cstdint>
#include <utility> // for the use of pair

class ITransport {
public:
    virtual void send_message(uint32_t procedure_id, const std::string& payload) = 0;
    virtual std::pair<uint32_t, std::string> receive_message() = 0;
    virtual std::string get_name() const = 0;
    virtual ~ITransport() = default;
};
