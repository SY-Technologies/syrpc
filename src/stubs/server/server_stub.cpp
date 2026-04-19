#include <iostream>
#include <stdexcept>
#include <thread>
#include "syrpc/stubs/server/server_stub.hpp"

ServerStub::ServerStub(ConnectionFactory factory) : factory(std::move(factory)) {};

void ServerStub::register_handler(uint32_t procedure_id, Handler handler)
{
    dispatcher.register_handler(procedure_id, handler);
};

void ServerStub::start()
{   

    while (true)
    {
        auto client = factory();
        std::cout << "Accepted connection from " << client->get_name() << std::endl;
        std::thread connection_thread([client = std::move(client), this]() mutable
                      { this->handle_client(std::move(client)); });
        connection_thread.detach();
    };
}

void ServerStub::handle_client(std::unique_ptr<ITransport> client)
{
    try
    {
        // Keep reading from this client until disconnect/error.
        while (true)
        {
            const auto [procedure_id, payload] = client->receive_message();
            std::cout << "Received message (procedure_id=" << procedure_id
                      << "): " << payload << std::endl;
            std::string response = dispatcher.dispatch(procedure_id, payload);
            client->send_message(procedure_id, response);
        }
    }
    catch (const std::exception &ex)
    {
        std::cout << "Client " << client->get_name()
                  << " disconnected: " << ex.what() << std::endl;
    }
}
