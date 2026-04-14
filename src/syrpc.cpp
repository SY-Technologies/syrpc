#include <iostream>
#include <memory>
#include <string>

#include "server_stub.hpp"
#include "tcp_transport.hpp"

int main(int argc, char* argv[]) {
    try {
        const uint16_t port = (argc >= 2) ? static_cast<uint16_t>(std::stoi(argv[1])) : 8080;

        auto listener = std::make_shared<TCPTransport>();
        listener->bind(port);
        listener->listen();

        ConnectionFactory factory = [listener]() -> std::unique_ptr<ITransport> {
            TCPTransport accepted = listener->accept();
            return std::make_unique<TCPTransport>(std::move(accepted));
        };

        ServerStub stub(factory);
        stub.register_handler(1, [](const std::string& payload) {
            return std::string("echo: ") + payload;
        });

        std::cout << "syrpc_server listening on port " << port << std::endl;
        stub.start();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "syrpc_server error: " << ex.what() << std::endl;
        return 1;
    }
}
