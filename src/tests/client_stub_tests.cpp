#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "syrpc/stubs/client/client_stub.hpp"
#include "syrpc/transport/itransport.hpp"

class FakeClientTransport : public ITransport {
public:
    uint32_t last_sent_proc_id = 0;
    std::string last_sent_payload;
    std::pair<uint32_t, std::string> next_response{0, ""};

    void send_message(uint32_t procedure_id, const std::string& payload) override {
        last_sent_proc_id = procedure_id;
        last_sent_payload = payload;
    }

    std::pair<uint32_t, std::string> receive_message() override {
        return next_response;
    }

    std::string get_name() const override { return "fake-client"; }
};

TEST(ClientStubTest, CallSendsRequestAndReturnsMatchingResponsePayload) {
    auto transport = std::make_unique<FakeClientTransport>();
    transport->next_response = {12, "SYRPC-we-rollin-baby"};

    FakeClientTransport* raw = transport.get(); // grab the raw pointer to inspect after move
    ClientStub stub(std::move(transport));
    // call() sends the message and receives a response, so we hijack the response function to return next_response
    // this is sound because if receive_message gets called, we know that the whole flow of call() works
    const std::string result = stub.call(12, "hello");

    EXPECT_EQ(raw->last_sent_proc_id, 12u);
    EXPECT_EQ(raw->last_sent_payload, "hello");
    EXPECT_EQ(result, "SYRPC-we-rollin-baby");
}

TEST(ClientStubTest, CallReturnsEmptyStringWhenResponseProcedureIdDoesNotMatch) {
    auto transport = std::make_unique<FakeClientTransport>();
    transport->next_response = {99, "wrong-id-response"};

    ClientStub stub(std::move(transport));

    EXPECT_EQ(stub.call(12, "hello"), "");
}
