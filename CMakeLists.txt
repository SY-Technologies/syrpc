cmake_minimum_required(VERSION 3.23)

project(SYRPC VERSION 1.0.0 LANGUAGES CXX)

# loads standard installation directory variables such as
# CMAKE_INSTALL_BINDIR (bin)
# CMAKE_INSTALL_LIBDIR (lib or lib64)
# CMAKE_INSTALL_INCLUDEDIR (include)
include(GNUInstallDirs)
#This brings in helper commands for generating package config files 
# so other CMake projects can do `find_package(SYRPC)`
include(CMakePackageConfigHelpers)

# Default to an optimized (release) build for single-config generators.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#asks for standard C++, not compiler-specific dialect extensions
# disabling compiler specific extensions to make the code more portable
set(CMAKE_CXX_EXTENSIONS OFF)
#to generate a compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SYRPC_BUILD_SHARED "Build SYRPC as a shared library (OFF builds static)." OFF)
option(SYRPC_ENABLE_WARNINGS "Enable strict compiler warnings." ON)
option(SYRPC_BUILD_DEV_BINARIES "Build local server/client test binaries." ON)

# The library can be built either static or shared from the same source set.
set(SYRPC_LIBRARY_TYPE STATIC)
if(SYRPC_BUILD_SHARED)
    set(SYRPC_LIBRARY_TYPE SHARED)
endif()

# -----------------------------
# Library target
# -----------------------------
# Serializer-agnostic core:
# - transport abstraction + TCP implementation
# - dispatcher and request/response stubs

#define a target named "syrpc"
add_library(syrpc ${SYRPC_LIBRARY_TYPE}
    src/transport/tcp_transport.cpp
    src/dispatcher/dispatcher.cpp
    src/stubs/client/client_stub.cpp
    src/stubs/server/server_stub.cpp
)

# Create a namespaced alias targed
add_library(SYRPC::syrpc ALIAS syrpc)

set_target_properties(syrpc PROPERTIES
    OUTPUT_NAME syrpc
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

#The include paths that should be used when compiling this target
target_include_directories(syrpc
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/transport>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/dispatcher>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/stubs/client>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/stubs/server>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/syrpc/transport>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/syrpc/dispatcher>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/syrpc/stubs/client>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/syrpc/stubs/server>
)

if(SYRPC_ENABLE_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(syrpc PRIVATE -Wall -Wextra -Wpedantic -Wno-deprecated)
    elseif(MSVC)
        target_compile_options(syrpc PRIVATE /W4)
    endif()
endif()

if(SYRPC_BUILD_DEV_BINARIES)
    add_executable(syrpc_server src/syrpc.cpp)
    add_executable(syrpc_client src/client.cpp)

    target_link_libraries(syrpc_server PRIVATE SYRPC::syrpc)
    target_link_libraries(syrpc_client PRIVATE SYRPC::syrpc)

    if(SYRPC_ENABLE_WARNINGS)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(syrpc_server PRIVATE -Wall -Wextra -Wpedantic -Wno-deprecated)
            target_compile_options(syrpc_client PRIVATE -Wall -Wextra -Wpedantic -Wno-deprecated)
        elseif(MSVC)
            target_compile_options(syrpc_server PRIVATE /W4)
            target_compile_options(syrpc_client PRIVATE /W4)
        endif()
    endif()
endif()

# -----------------------------
# Install + package export
# -----------------------------
install(TARGETS syrpc
    EXPORT SYRPCTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES src/transport/itransport.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/syrpc/transport)
install(FILES src/transport/tcp_transport.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/syrpc/transport)
install(FILES src/dispatcher/dispatcher.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/syrpc/dispatcher)
install(FILES src/stubs/client/client_stub.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/syrpc/stubs/client)
install(FILES src/stubs/server/server_stub.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/syrpc/stubs/server)

install(EXPORT SYRPCTargets
    FILE SYRPCTargets.cmake
    NAMESPACE SYRPC::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SYRPC
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/SYRPCConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SYRPCConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SYRPC
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/SYRPCConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SYRPCConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SYRPCConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SYRPC
)

#----------GOOGLE TEST SETUP----------------------
# Note to self: https://google.github.io/googletest/quickstart-cmake.html
include(CTest) # defines BUILD_TESTING option --> https://cmake.org/cmake/help/latest/module/CTest.html

if(BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  )
  # For MSVC runtime consistency, # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_executable(syrpc_unit_tests
    src/tests/dispatcher_tests.cpp
    src/tests/client_stub_tests.cpp
    src/tests/server_stub_tests.cpp
    src/tests/tcp_transport_tests.cpp
    
  )
  target_link_libraries(syrpc_unit_tests
    PRIVATE
      SYRPC::syrpc
      GTest::gtest_main
  )

  if(SYRPC_BUILD_DEV_BINARIES)
    add_executable(syrpc_e2e_tests
      src/tests/syrpc_e2e_tests.cpp
    )
    target_link_libraries(syrpc_e2e_tests
      PRIVATE
        GTest::gtest_main
    )
    target_compile_definitions(syrpc_e2e_tests
      PRIVATE
        SYRPC_SERVER_BIN="$<TARGET_FILE:syrpc_server>"
        SYRPC_CLIENT_BIN="$<TARGET_FILE:syrpc_client>"
    )
    add_dependencies(syrpc_e2e_tests syrpc_server syrpc_client)
  endif()

  include(GoogleTest)
  gtest_discover_tests(syrpc_unit_tests)
  if(TARGET syrpc_e2e_tests)
    gtest_discover_tests(syrpc_e2e_tests)
  endif()
endif()


message(STATUS "Configured SYRPC ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Library type: ${SYRPC_LIBRARY_TYPE}")
message(STATUS "Dev binaries: ${SYRPC_BUILD_DEV_BINARIES}")
