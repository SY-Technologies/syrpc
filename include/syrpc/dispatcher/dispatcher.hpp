#pragma once

#include <functional>
#include <map>
#include <string>

// handlers take the payload string and return a string
using Handler = std::function<std::string(const std::string&)>;

class Dispatcher {
public:
    Dispatcher();
    ~Dispatcher();
    void register_handler(uint32_t procedure_id, Handler handler);
    std::string dispatch(uint32_t procedure_id, const std::string& payload);

private:
    std::map<uint32_t, Handler> handlers_map;
};

