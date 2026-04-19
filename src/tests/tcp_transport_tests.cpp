#include <gtest/gtest.h>
#include <future>
#include <thread>
#include <string>
#include <chrono>
#include <stdexcept>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "syrpc/transport/tcp_transport.hpp"

// Helper: reserve a free localhost port for tests.
static uint16_t find_free_port() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) throw std::runtime_error("socket failed");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);// maps specifically to 127.0.0.1 and only accept connections from the local machine
    addr.sin_port = htons(0); // dynamic ephemeral port

    //https://en.cppreference.com/w/cpp/language/reinterpret_cast.html
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        throw std::runtime_error("bind failed");
    }

    socklen_t len = sizeof(addr);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
        ::close(fd);
        throw std::runtime_error("getsockname failed");
    }

    uint16_t port = ntohs(addr.sin_port);
    ::close(fd);
    return port;
}

static void set_promise_value_if_possible(std::promise<std::string>& p, const std::string& value) noexcept {
    try {
        p.set_value(value);
    } catch (...) {
        // Promise may already be satisfied; ignore in tests.
    }
}

TEST(TCPTransportTest, ClientServerRoundTripMessage) {
    uint16_t port = 0;
    try {
        port = find_free_port();
    } catch (const std::exception& ex) {
        GTEST_SKIP() << "Skipping network test: " << ex.what();
    }

    /**
     * On-shot communication between two test threads here:
     * `promise server_ready` is a signal to be sent when done
     * `future ready`is a signal waiting for the ready signal
     */
    std::promise<void> server_ready;
    std::promise<std::string> server_error;
    std::future<void> ready = server_ready.get_future(); //create receiver for the server_ready signal
    auto setup_server = [&]() noexcept {
        try {
            TCPTransport listener;
            listener.bind(port);
            listener.listen();//open the connection queue
            server_ready.set_value(); // unblock main thread

            TCPTransport peer = listener.accept();//start accepting connections
            auto [proc_id, payload] = peer.receive_message();
            EXPECT_EQ(proc_id, 10u);
            EXPECT_EQ(payload, "Bonjour");

            peer.send_message(proc_id, "Bonsoir");
            set_promise_value_if_possible(server_error, "");
        } catch (const std::exception& ex) {
            set_promise_value_if_possible(server_error, ex.what());
        } catch (...) {
            set_promise_value_if_possible(server_error, "unknown server thread error");
        }
    };
    std::thread server(setup_server);

    //main thread waits here until server is ready
    const auto ready_status = ready.wait_for(std::chrono::seconds(2));
    EXPECT_EQ(ready_status, std::future_status::ready);

    if (ready_status == std::future_status::ready) {
        try {
            TCPTransport client;
            client.connect("127.0.0.1", port);
            client.send_message(10, "Bonjour");
            auto [resp_id, resp_payload] = client.receive_message();

            EXPECT_EQ(resp_id, 10u);
            EXPECT_EQ(resp_payload, "Bonsoir");
        } catch (const std::exception& ex) {
            ADD_FAILURE() << "Client path failed: " << ex.what();
        }
    }

    server.join(); //Wait until the server thread finishes before exiting
    EXPECT_TRUE(server_error.get_future().get().empty());
}

TEST(TCPTransportTest, ConnectInvalidHostThrows) {
    TCPTransport client;
    EXPECT_THROW(client.connect("some-wilddd-ip", 8080), std::runtime_error);
}

TEST(TCPTransportTest, GetNameIsPopulatedAfterConnectAndAccept) {
    uint16_t port = 0;
    try {
        port = find_free_port();
    } catch (const std::exception& ex) {
        GTEST_SKIP() << "Skipping network test: " << ex.what();
    }

    std::promise<std::string> accepted_name;
    std::promise<void> server_ready;
    std::promise<std::string> server_error;
    auto server_setup = [&]() noexcept {
        try {
            TCPTransport listener;
            listener.bind(port);
            listener.listen();
            server_ready.set_value();
            TCPTransport peer = listener.accept();
            set_promise_value_if_possible(accepted_name, peer.get_name());
            set_promise_value_if_possible(server_error, "");
        } catch (const std::exception& ex) {
            set_promise_value_if_possible(accepted_name, "unknown");
            set_promise_value_if_possible(server_error, ex.what());
        } catch (...) {
            set_promise_value_if_possible(accepted_name, "unknown");
            set_promise_value_if_possible(server_error, "unknown server thread error");
        }
    };
    std::thread server(server_setup);

    const auto ready_status = server_ready.get_future().wait_for(std::chrono::seconds(2));
    EXPECT_EQ(ready_status, std::future_status::ready);
    if (ready_status == std::future_status::ready) {
        try {
            TCPTransport client;
            client.connect("127.0.0.1", port);
            EXPECT_EQ(client.get_name(), "127.0.0.1:" + std::to_string(port));
        } catch (const std::exception& ex) {
            ADD_FAILURE() << "Client connect failed: " << ex.what();
        }
    }

    const std::string server_seen = accepted_name.get_future().get();
    EXPECT_FALSE(server_seen.empty());   // should be something like "127.0.0.1:<ephemeral>"
    EXPECT_NE(server_seen, "unknown");

    server.join();

    const std::string error = server_error.get_future().get();
    EXPECT_TRUE(error.empty()) << error;
}
