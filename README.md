# SYRPC: A Synchronous Transport Protocol-Agnostic Remote Procedure Call Framework
## What is This Project?

Remote Procedure Call (RPC) allows a program on one computer to invoke a function on another computer. Since the two machines do not share memory, parameters must be transmitted over the network in a way that is largely transparent to the caller.

SYRPC is my own exploration of building a protocol-agnostic RPC framework capable of handling both TCP and UDP in a configurable way. While robust RPC frameworks like gRPC already exist, SYRPC is an opportunity to deeply understand the underlying mechanisms of distributed communication. It’s ambitious, but as the saying goes: no pain, no gain.

The core library is serializer-agnostic: it only moves RPC payloads as `std::string`. Consumers are free to use protobuf, JSON, FlatBuffers, MessagePack, or any custom serializer.

## Why This Project?

During my final year at university, I took a course in distributed systems, which introduced me to remote procedure call frameworks and the Raft consensus algorith among other mind-blowing concepts. That experience sparked a strong interest in distributed systems, and I had the opportunity to build a small RPC framework with guided exercises. Implementing Raft on top of it was particularly rewarding.

SYRPC is my personal project to recreate and extend that experience, designing a modular, configurable RPC framework from scratch without guided exercises. It’s a hands-on way to explore distributed system design and C++ concurrency at a deeper level.

## Build Instructions

SYRPC is built as a reusable C++ library, with local dev binaries for testing:

- `libsyrpc.a` (static) or
- `libsyrpc.dylib` / `libsyrpc.so` (shared)
- `syrpc_server` (from `src/syrpc.cpp`)
- `syrpc_client` (from `src/client.cpp`)

### Prerequisites

- CMake 3.23+
- C++17 compiler (`clang++` or `g++`)

### Key Build Options

- `-DSYRPC_BUILD_SHARED=OFF|ON`
  - `OFF`: build static library
  - `ON`: build shared library
- `-DSYRPC_ENABLE_WARNINGS=ON|OFF`
- `-DSYRPC_BUILD_DEV_BINARIES=ON|OFF`
  - `ON`: build `syrpc_server` and `syrpc_client` for local testing

### Build (Static Library, Release)

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DSYRPC_BUILD_SHARED=OFF

cmake --build build --target syrpc --parallel
```

### Build Library + Local Dev Binaries (Debug)

```bash
./scripts/build --reconfigure --all
```

### Build (Shared Library, Release)

```bash
cmake -S . -B build-shared \
  -DCMAKE_BUILD_TYPE=Release \
  -DSYRPC_BUILD_SHARED=ON

cmake --build build-shared --target syrpc --parallel
```

### Install and Export Package

Install locally:

```bash
cmake --install build --prefix /usr/local
```

This installs:
- library files (`lib/`)
- public headers (`include/syrpc/...`)
- CMake package config files (`lib/cmake/SYRPC`)

### Use in Another CMake Project

This is the complete flow for reusing SYRPC in a different CMake project.

#### Step 1: Build and install SYRPC

From this SYRPC repository:

```bash
cmake -S . -B build \
  -DBUILD_TESTING=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DSYRPC_BUILD_SHARED=OFF

cmake --build build --target syrpc --parallel
cmake --install build --prefix /usr/local
```

You can replace `/usr/local` with a user-local prefix like `$HOME/.local` if you do not want a system-wide install.

#### Step 2: Add SYRPC to your consumer project CMake

In the other project's `CMakeLists.txt`:

```cmake
add_executable(my_app src/main.cpp)
find_package(SYRPC REQUIRED CONFIG)
target_link_libraries(my_app PRIVATE SYRPC::syrpc)
```

What each part means:

- `add_executable(my_app src/main.cpp)`
  - `my_app` is your executable target name. You choose it.
  - `src/main.cpp` is your source file list (one or many files).
- `find_package(SYRPC REQUIRED CONFIG)`
  - asks CMake to locate SYRPC's installed package config (`SYRPCConfig.cmake`).
  - `REQUIRED` means configuration should fail immediately if SYRPC is not found.
  - `CONFIG` means "use package config files" (the modern package export route).
- `target_link_libraries(my_app PRIVATE SYRPC::syrpc)`
  - links your target against the exported SYRPC library target.
  - `SYRPC::syrpc` is the namespaced target name exported by this repo.
  - `PRIVATE` means this dependency is used to build `my_app` only (standard choice for executables).

#### Step 3: Tell CMake where SYRPC is installed (if needed)

If SYRPC was installed in a non-default location, pass the prefix while configuring the consumer project:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/usr/local
cmake --build build --parallel
```

If you installed to `$HOME/.local`, use:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=$HOME/.local
cmake --build build --parallel
```

Notes:

- CMake looks for `.../lib/cmake/SYRPC/SYRPCConfig.cmake` under each prefix.
- Some systems may find `/usr/local` automatically; `CMAKE_PREFIX_PATH` makes it explicit and reproducible.

#### Step 4: Use SYRPC headers in your code

Use the namespaced public headers:

```cpp
#include "syrpc/transport/itransport.hpp"
#include "syrpc/transport/tcp_transport.hpp"
#include "syrpc/dispatcher/dispatcher.hpp"
#include "syrpc/stubs/client/client_stub.hpp"
#include "syrpc/stubs/server/server_stub.hpp"
```

#### Step 5: Static vs shared in downstream projects

- If SYRPC is installed as static (`-DSYRPC_BUILD_SHARED=OFF`):
  - simplest runtime behavior (no shared library loader issues)
  - larger consumer binary size
- If SYRPC is installed as shared (`-DSYRPC_BUILD_SHARED=ON`):
  - smaller binaries, shared upgrades possible
  - you must ensure runtime loader can find `libsyrpc` (`.dylib`/`.so`) on target machines

#### Quick troubleshooting for consumer setup

- `Could not find a package configuration file provided by "SYRPC"`:
  - verify install was run
  - verify prefix passed via `-DCMAKE_PREFIX_PATH=...`
- Link errors mentioning unresolved SYRPC symbols:
  - confirm `target_link_libraries(... SYRPC::syrpc)` is on the executable/library that uses SYRPC
- Include errors for `syrpc/...` headers:
  - ensure `find_package` succeeded
  - do not include old flat header names; use `syrpc/...` paths

## Usage Guide

This section is for teams that want to embed SYRPC into an existing service or application and move quickly without committing to a specific serialization format.

### What SYRPC Handles (and What It Doesn't)

SYRPC is intentionally narrow in scope:

- It gives you transport + dispatch + client/server stubs.
- It sends and receives payloads as `std::string`.
- It routes requests by `procedure_id`.

SYRPC does not impose a schema language, IDL compiler, or message format. You choose those in your own project.

In practice, this means:

- If your org already uses protobuf: keep using protobuf.
- If you want JSON for readability: use JSON.
- If you need compact binary payloads: use MessagePack/FlatBuffers/custom binary.

The library stays the same in all three cases.

### Consumer Project Layout (Recommended)

A simple pattern that scales well is:

- one shared header for procedure IDs
- one serializer adapter module
- one RPC client wrapper per remote service
- one RPC server registration function per service

For example:

```text
my_app/
  include/
    rpc_ids.hpp
  src/
    rpc/
      serializer_json.cpp
      user_service_client.cpp
      user_service_server.cpp
```

### 1) Define Procedure IDs in One Place

Keep IDs centralized so client and server cannot drift silently.

```cpp
// rpc_ids.hpp
#pragma once
#include <cstdint>

namespace rpc_ids {
constexpr std::uint32_t kEcho = 1;
constexpr std::uint32_t kCreateUser = 100;
constexpr std::uint32_t kGetUser = 101;
}
```

### 2) Minimal Server Integration

This is the standard pattern on the server side:

1. Bind and listen with `TCPTransport`.
2. Create a `ConnectionFactory`.
3. Register handlers on `ServerStub`.
4. Start the server loop.

```cpp
#include <memory>
#include <string>
#include "syrpc/stubs/server/server_stub.hpp"
#include "syrpc/transport/tcp_transport.hpp"
#include "rpc_ids.hpp"

int main() {
    auto listener = std::make_shared<TCPTransport>();
    listener->bind(8080);
    listener->listen();

    ConnectionFactory factory = [listener]() -> std::unique_ptr<ITransport> {
        TCPTransport accepted = listener->accept();
        return std::make_unique<TCPTransport>(std::move(accepted));
    };

    ServerStub stub(factory);

    stub.register_handler(rpc_ids::kEcho, [](const std::string& payload) {
        return std::string("echo: ") + payload;
    });

    // Example JSON handler:
    // - parse JSON payload
    // - run business logic
    // - return JSON response
    stub.register_handler(rpc_ids::kCreateUser, [](const std::string& payload) {
        // deserialize(payload) -> request DTO
        // validate + execute domain logic
        // serialize(response DTO)
        return R"({"ok":true})";
    });

    stub.start();
}
```

### 3) Minimal Client Integration

Client flow is:

1. Connect transport.
2. Build `ClientStub`.
3. Serialize request.
4. Call with `procedure_id`.
5. Deserialize response.

```cpp
#include <memory>
#include <string>
#include "syrpc/stubs/client/client_stub.hpp"
#include "syrpc/transport/tcp_transport.hpp"
#include "rpc_ids.hpp"

int main() {
    auto transport = std::make_unique<TCPTransport>();
    transport->connect("127.0.0.1", 8080);

    ClientStub stub(std::move(transport));

    // Example payload can be protobuf bytes, JSON text, etc.
    const std::string request_payload = R"({"email":"test@example.com"})";
    const std::string response_payload =
        stub.call(rpc_ids::kCreateUser, request_payload);

    // deserialize(response_payload)
}
```

### 4) Serializer Integration Pattern

Treat serialization as an adapter layer around `stub.call(...)`.

```cpp
template <typename Req, typename Res>
Res call_rpc(ClientStub& stub, std::uint32_t procedure_id, const Req& request) {
    const std::string payload = serialize(request);      // your code
    const std::string raw = stub.call(procedure_id, payload);
    return deserialize<Res>(raw);                        // your code
}
```

This keeps serialization choice outside SYRPC and makes switching formats possible without transport rewrites.

### 5) Error Handling Expectations

Networking and socket-level failures are reported as exceptions from transport operations.

Practical guidance:

- wrap your top-level server/client entrypoint in `try/catch`
- log procedure IDs and peer names for failures
- apply your own retry policy in the client wrapper layer

Current behavior for `ClientStub::call`:

- it returns response payload when response `procedure_id` matches request `procedure_id`
- otherwise it returns an empty string

If empty string is ambiguous in your domain, wrap `ClientStub` in a typed result (`status + payload`) in your app layer.

### 6) Concurrency Notes

`ServerStub` handles each accepted connection in its own detached thread.

Implications for consumers:

- your handler code must be thread-safe
- shared mutable state must be synchronized
- long-running handlers should not block unrelated requests if you can avoid it

If you need stronger lifecycle control (graceful shutdown, bounded worker pools), add that policy in your server host process around SYRPC.

### 7) Production Checklist

Before shipping a service built on SYRPC, verify:

1. Procedure IDs are centrally defined and versioned.
2. Serialization and validation are explicit for every handler.
3. All handler exceptions are caught and logged with context.
4. Timeouts/retries are defined in the client wrapper.
5. Integration tests include at least one real client/server round-trip.
6. Port binding, deployment permissions, and firewall rules are documented.

### 8) Include Paths After Install

`SYRPC::syrpc` exports include directories for installed headers. In most consumer projects, these includes work directly:

```cpp
#include "syrpc/transport/itransport.hpp"
#include "syrpc/transport/tcp_transport.hpp"
#include "syrpc/dispatcher/dispatcher.hpp"
#include "syrpc/stubs/client/client_stub.hpp"
#include "syrpc/stubs/server/server_stub.hpp"
```

If your project enforces namespaced includes, create thin forwarding headers in your own tree (for example `myapp/rpc/...`) and keep SYRPC headers internal.

### Local Dev Run Commands

```bash
# Run server on port 8080
./scripts/run --server

# Run client against 127.0.0.1:8080
./scripts/run --client

# Custom port/host
./scripts/run --server --port 9000
./scripts/run --client --host 127.0.0.1 --port 9000
```

### Script Reference

```bash
# Build helpers
./scripts/build --all
./scripts/build --target syrpc
./scripts/build --target syrpc_server
./scripts/build --target syrpc_client

# Run helper
./scripts/run --server
./scripts/run --client
./scripts/run --build --server
./scripts/run --no-build --client
```

### Testing (GoogleTest)

SYRPC tests use GoogleTest through CMake.

1. Enable tests at configure time:

```bash
cmake -S . -B build -DBUILD_TESTING=ON
```

2. Build test targets:

a. To build the unit tests
```bash
cmake --build build --target syrpc_unit_tests --parallel
```
b. To build the e2e tests
```bash
cmake --build build --target syrpc_e2e_tests --parallel
```
c. To build both unit and e2e tests
```bash
cmake --build build --target syrpc_unit_tests syrpc_e2e_tests --parallel
```

3. Run all discovered tests with CTest:

```bash
ctest --test-dir build --output-on-failure
```

4. Run only tests that match a regex:

```bash
ctest --test-dir build -R Dispatcher --output-on-failure
ctest --test-dir build -R TCPTransport --output-on-failure
```

Notes:
- If `BUILD_TESTING` is `OFF`, test targets are not generated.
- Typical test target names used in this project:
  - `syrpc_unit_tests`
  - `syrpc_transport_tests`
  - `syrpc_e2e_tests`

### Troubleshooting

- Link/include errors in consumers:
  - link against `SYRPC::syrpc`
  - include headers from `include/syrpc/...` after install
- Build type not set:
  - set `-DCMAKE_BUILD_TYPE=Release` for release artifacts
