# SYRPC: A Protocol-Agnostic Remote Procedure Call Framework
## What is This Project?

Remote Procedure Call (RPC) allows a program on one computer to invoke a function on another computer. Since the two machines do not share memory, parameters must be transmitted over the network in a way that is largely transparent to the caller.

SYRPC is my own exploration of building a protocol-agnostic RPC framework capable of handling both TCP and UDP in a configurable way. While robust RPC frameworks like gRPC already exist, SYRPC is an opportunity to deeply understand the underlying mechanisms of distributed communication. It’s ambitious, but as the saying goes: no pain, no gain.

## Why This Project?

During my final year at university, I took a course in distributed systems, which introduced me to remote procedure call frameworks and the Raft consensus algorithm. That experience sparked a strong interest in distributed systems, and I had the opportunity to build a small RPC framework with guided exercises. Implementing Raft on top of it was particularly rewarding.

SYRPC is my personal project to recreate and extend that experience, designing a modular, configurable RPC framework from scratch without guided exercises. It’s a hands-on way to explore distributed system design and C++ concurrency at a deeper level.

