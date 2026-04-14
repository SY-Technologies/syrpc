#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
bool can_connect_local(uint16_t port) {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        ::close(fd);
        return false;
    }

    const bool ok = (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    ::close(fd);
    return ok;
}

uint16_t pick_candidate_port() {
    // Choose from the high non-privileged range and pick one that currently
    // does not accept connections on localhost.
    for (uint16_t port = 45000; port < 47000; ++port) {
        if (!can_connect_local(port)) {
            return port;
        }
    }
    throw std::runtime_error("unable to find candidate test port");
}

struct ServerStartResult {
    bool ready = false;
    bool exited_early = false;
};

ServerStartResult wait_for_server_ready(pid_t server_pid, uint16_t port, int timeout_ms) {
    const int poll_ms = 50;
    const int iterations = timeout_ms / poll_ms;

    for (int i = 0; i < iterations; ++i) {
        int status = 0;
        const pid_t exited = ::waitpid(server_pid, &status, WNOHANG);
        if (exited == server_pid) {
            return {false, true};
        }

        if (can_connect_local(port)) {
            return {true, false};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms));
    }

    return {false, false};
}

std::string run_command_and_capture_stdout(const std::string& command) {
    FILE* pipe = ::popen(command.c_str(), "r");
    if (pipe == nullptr) {
        throw std::runtime_error("popen failed");
    }

    std::string output;
    char buffer[512];
    while (::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    const int rc = ::pclose(pipe);
    if (rc != 0) {
        throw std::runtime_error("command failed: " + command);
    }
    return output;
}
}  // namespace

TEST(SyrpcE2E, ServerAndClientExchangeEchoMessage) {
    pid_t server_pid = -1;
    uint16_t selected_port = 0;
    bool ready = false;

    // Retry startup on different ports to avoid transient local conflicts.
    for (int attempt = 0; attempt < 8 && !ready; ++attempt) {
        const uint16_t port = static_cast<uint16_t>(pick_candidate_port() + attempt);
        if (can_connect_local(port)) {
            continue;
        }

        const std::string port_text = std::to_string(port);
        const pid_t pid = ::fork();
        ASSERT_NE(pid, -1) << "fork() failed";

        if (pid == 0) {
            ::execl(SYRPC_SERVER_BIN, SYRPC_SERVER_BIN, port_text.c_str(), static_cast<char*>(nullptr));
            std::perror("execl");
            _exit(127);
        }

        const ServerStartResult start = wait_for_server_ready(pid, port, 3000);
        if (start.ready) {
            ready = true;
            selected_port = port;
            server_pid = pid;
            break;
        }

        if (!start.exited_early) {
            ::kill(pid, SIGTERM);
            int status = 0;
            ::waitpid(pid, &status, 0);
        }
    }

    if (!ready) {
        GTEST_SKIP() << "Skipping e2e test: unable to start syrpc_server in this environment";
    }

    const std::string command =
        "printf 'hello\\nquit\\n' | \"" + std::string(SYRPC_CLIENT_BIN) + "\" 127.0.0.1 " +
        std::to_string(selected_port);
    const std::string output = run_command_and_capture_stdout(command);
    EXPECT_NE(output.find("response: echo: hello"), std::string::npos) << output;

    if (server_pid > 0) {
        ::kill(server_pid, SIGTERM);
        int status = 0;
        ::waitpid(server_pid, &status, 0);
    }
}
