#include <iostream>
#include <memory>
#include <string>

#include "syrpc/stubs/client/client_stub.hpp"
#include "syrpc/transport/tcp_transport.hpp"

int main(int argc, char* argv[]) {
    try {
        const std::string host = (argc >= 2) ? argv[1] : "127.0.0.1";
        const uint16_t port = (argc >= 3) ? static_cast<uint16_t>(std::stoi(argv[2])) : 8080;

        auto transport = std::make_unique<TCPTransport>();
        transport->connect(host, port);

        ClientStub stub(std::move(transport));

        std::cout << "Connected to " << host << ":" << port
                  << ". Type a message, or 'quit' to exit." << std::endl;

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == "quit") {
                break;
            }
            const std::string response = stub.call(1, line);
            std::cout << "response: " << response << std::endl;
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "syrpc_client error: " << ex.what() << std::endl;
        return 1;
    }
}
