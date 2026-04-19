#pragma once

#include <functional>
#include <memory>
#include <string>

#include "syrpc/dispatcher/dispatcher.hpp"
#include "syrpc/transport/itransport.hpp"

using Handler = std::function<std::string(const std::string&)>;
using ConnectionFactory = std::function<std::unique_ptr<ITransport>()>;

class ServerStub {
public:
    ServerStub(ConnectionFactory factory);
    ServerStub() = default;
    ~ServerStub() = default;
    void register_handler(uint32_t procedure_id, Handler handler);
    void start();

private:
    ConnectionFactory factory;
    Dispatcher dispatcher;
    void handle_client(std::unique_ptr<ITransport> client);
};

