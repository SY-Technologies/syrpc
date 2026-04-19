#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "syrpc/transport/itransport.hpp"

class ClientStub {
public:
    ClientStub(std::unique_ptr<ITransport> transport);
    ~ClientStub() = default;
    const std::string call(uint32_t procedure_id, const std::string& payload);

private:
    std::unique_ptr<ITransport> transport;
    std::string server_host;
};

