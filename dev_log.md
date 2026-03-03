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