// server_stub_test.cpp
#include <gtest/gtest.h>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "server_stub.hpp"
#include "itransport.hpp"

struct StopServerLoop : public std::runtime_error {
    StopServerLoop() : std::runtime_error("stop server loop") {}
};

struct SharedTransportState {
    std::mutex mu;
    std::condition_variable cv;
    bool response_sent = false;

    std::vector<std::pair<uint32_t, std::string>> incoming;
    size_t read_index = 0;
    std::vector<std::pair<uint32_t, std::string>> outgoing;
};

class FakeServerTransport : public ITransport {
public:
    explicit FakeServerTransport(std::shared_ptr<SharedTransportState> s, std::string n = "fake-peer")
        : state(std::move(s)), name(std::move(n)) {}

    void send_message(uint32_t procedure_id, const std::string& payload) override {
        {
            std::lock_guard<std::mutex> lock(state->mu);
            state->outgoing.push_back({procedure_id, payload});
            state->response_sent = true;
        }
        state->cv.notify_all();
    }

    std::pair<uint32_t, std::string> receive_message() override {
        std::lock_guard<std::mutex> lock(state->mu);
        if (state->read_index >= state->incoming.size()) {
            throw std::runtime_error("client disconnected");
        }
        return state->incoming[state->read_index++];
    }

    std::string get_name() const override { return name; }

private:
    std::shared_ptr<SharedTransportState> state;
    std::string name;
};

TEST(ServerStubTest, StartAcceptsClientDispatchesAndSendsResponse) {
    auto state = std::make_shared<SharedTransportState>();
    state->incoming.push_back({7, "ping"});

    int factory_calls = 0;
    ConnectionFactory factory = [state, &factory_calls]() -> std::unique_ptr<ITransport> {
        ++factory_calls;
        if (factory_calls == 1) {
            return std::make_unique<FakeServerTransport>(state, "client-1");
        }
        throw StopServerLoop(); // break ServerStub::start() infinite accept loop
    };

    ServerStub stub(factory);
    stub.register_handler(7, [](const std::string& payload) {
        return std::string("echo:") + payload;
    });

    std::thread t([&]() {
        try {
            stub.start();
            FAIL() << "Expected StopServerLoop to stop start()";
        } catch (const StopServerLoop&) {
            // expected
        }
    });

    // wait until detached client thread sends response
    {
        std::unique_lock<std::mutex> lock(state->mu);
        ASSERT_TRUE(state->cv.wait_for(lock, std::chrono::seconds(2),
            [&]() { return state->response_sent; }));
        ASSERT_EQ(state->outgoing.size(), 1u);
        EXPECT_EQ(state->outgoing[0].first, 7u);
        EXPECT_EQ(state->outgoing[0].second, "echo:ping");
    }

    t.join();
}

TEST(ServerStubTest, MissingHandlerDoesNotSendResponse) {
    auto state = std::make_shared<SharedTransportState>();
    state->incoming.push_back({999, "no-handler-for-this"});

    int factory_calls = 0;
    ConnectionFactory factory = [state, &factory_calls]() -> std::unique_ptr<ITransport> {
        ++factory_calls;
        if (factory_calls == 1) {
            return std::make_unique<FakeServerTransport>(state);
        }
        throw StopServerLoop();
    };

    ServerStub stub(factory);

    std::thread t([&]() {
        try {
            stub.start();
            FAIL() << "Expected StopServerLoop";
        } catch (const StopServerLoop&) {
            // expected
        }
    });

    // give detached handler thread time to process and exit on exception
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    {
        std::lock_guard<std::mutex> lock(state->mu);
        EXPECT_TRUE(state->outgoing.empty());
    }

    t.join();
}
