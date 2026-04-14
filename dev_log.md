# Dev Diary — Development Notes

## Sunday, March 1 2026

### Focus
Setting up the project build system and integrating Protobuf using CMake.

### Problem
Integrating Protobuf through CMake's `FetchContent` proved more complex than expected. Getting the dependency to build cleanly and understanding how external projects expose targets turned into a larger challenge than anticipated. I am still not fully comfortable with CMake, so progress has been intentionally slow while I focus on understanding the fundamentals instead of copying configurations blindly.

### Investigation
I spent time learning how modern CMake manages dependencies using target-based linking rather than global include or linker settings. Several C++ courses I took in university provided their own Makefiles, which worked well but largely shielded me from learning how build systems actually function. This is the first time I am designing the build process myself and understanding how dependencies propagate through targets.

My distributed systems course also did not use Protobuf because it was taught in Go, where JSON marshalling and unmarshalling were built into the workflow. Working in C++ makes serialization and dependency management much more explicit, which is forcing me to learn the tooling properly.

### Decision
I decided to integrate Protobuf using `FetchContent` instead of relying on a system-installed version. The goal is to keep the project as plug and play as possible so that cloning the repository is enough to build it. This also supports my longer term goal of running the project on a Raspberry Pi, where minimizing manual setup and environment differences becomes important. I am leaning toward static linking to reduce runtime dependency issues across platforms.

### Takeaway
CMake makes a lot more sense once I stopped thinking in terms of include folders and linker flags and started thinking in terms of targets depending on other targets. It is still confusing, but the mental model is slowly clicking. Most of today was less about writing code and more about learning how the project actually gets built.One think that did the heavy lifting here is that I had a good understanding of compilation and linking before tackling CMake.


### Next Steps
- Complete Protobuf integration
- Generate `.proto` sources automatically
- Begin designing the SYRPC message schema

## Monday, March 2

Today, I tackled a pain point that I was dealing with. I wanted to simplify the commands I use to run or build the project and my research landed on [direnv](https://direnv.net/), a very convenient tool for setting up custom commands for specific directories. After about 1h of struggling, I managed to get it to work. The outcome is that now I can simply type `build`or `run`to run or build the project.

## Friday, April 10th
Back to this project after about a month off. The break was not planned, but it helped.

I decided to stop treating this as only an RPC exercise and try a real use case on top: remote file operations (from laptop to Raspberry Pi). I spent most of the day finishing protobuf setup and defining message shapes for file operations.

I also pushed transport forward. I can now send framed messages between two local terminals over TCP. That part was important because I finally have a clean packet shape (`procedure_id` + payload length + payload) and not just ad-hoc string passing.

Still a lot left, but today felt like real progress again.

## Saturday, April 11th
Big day.

I finished the transport layer and wrote both stubs. At that point I was about to continue with file-system logic, but I changed direction.

I want this RPC library to be reusable in other projects, so I pulled file-service concerns out of this repo. This repo should stay focused on RPC core only. You will not see any trace of it ( file system stuff) because it never got committed.

That meant cleaning boundaries:

- transport handles socket send/receive only
- dispatcher/stubs handle routing by procedure id
- serializer choice belongs to consumer projects

I struggled quite a bit with protobuf + CMake, but that time was not wasted. Even if protobuf is no longer in this core lib, I now know how to integrate it when I need it in projects that sit on top of this.


# Sunday,April 12th
Testing day.

I integrated GoogleTest into CMake and got tests running through `ctest`. Writing tests was straightforward. The hard part was build wiring and making targets discoverable in a clean way.

I now have coverage across:

- dispatcher behavior
- stub behavior
- TCP transport behavior
- one end-to-end server/client path

Main lesson today: network tests get flaky fast if startup order and thread errors are not handled carefully. I had to fix that so failures are deterministic and useful.

Overall, good day. Testing is now part of the normal build flow instead of something separate. Also, body are tests fast in C++! A test suite in most other languages I have used usually is quite slow but in the blink of an eye, the RPC test are done running!
