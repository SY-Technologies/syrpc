#include <map>
#include <utility>
#include <memory>
#include "syrpc/stubs/client/client_stub.hpp"

ClientStub::ClientStub(std::unique_ptr<ITransport> transport) : transport(std::move(transport)) {};

const std::string ClientStub::call(uint32_t procedure_id, const std::string &payload)
{
    transport->send_message(procedure_id, payload);

    std::pair<uint32_t, std::string> response = transport->receive_message();
    const std::string &response_payload = response.second;
    uint32_t response_proc_id = response.first;

    if (response_proc_id == procedure_id)
    {
        return response_payload;
    }

    return "";
}
