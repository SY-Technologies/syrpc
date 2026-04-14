#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include "dispatcher.hpp"

//TEST(TestSuiteName,TestName)
TEST(DispatcherTest, DispatchCallsRegisteredHandler) {
    Dispatcher d;
    auto test_handler = [](const std::string& payload) {
        return std::string("echo:") + payload;
    };
    d.register_handler(2026, test_handler);

    EXPECT_EQ(d.dispatch(2026, "hello"), "echo:hello");
}

TEST(DispatcherTest, RegisterHandlerOverridesPreviousHandlerForSameId) {
    Dispatcher d;
    auto first_handler = [](const std::string&) { return "first"; };
    auto second_handler = [](const std::string&) { return "second"; };
    auto handler_id = 1;
    d.register_handler(handler_id, first_handler);
    d.register_handler(handler_id, second_handler);

    EXPECT_EQ(d.dispatch(handler_id, "ignored"), "second");
}

TEST(DispatcherTest, DispatchThrowsWhenProcedureIsMissing) {
    Dispatcher d;
    //Trying to dispatch a handler that was not registered
    EXPECT_THROW(d.dispatch(999, "payload"), std::runtime_error);
}

TEST(DispatcherTest, DispatchPassesPayloadUnmodified) {
    Dispatcher d;
    auto handler = [](const std::string& payload) { return payload; };
    d.register_handler(7, handler);

    const std::string input = "abc 123 !@#";
    EXPECT_EQ(d.dispatch(7, input), input);
}