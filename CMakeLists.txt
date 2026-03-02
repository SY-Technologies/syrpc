cmake_minimum_required(VERSION 3.23)

# My intention being to be able to run this on both my mac and my Raspberry pi,
# I wanted to make the build system flexible but I personally prefer Clang as my project compiler
# Check for environment overrides first
if(DEFINED ENV{SY_CXX})
    set(CMAKE_CXX_COMPILER $ENV{SY_CXX} CACHE STRING "C++ compiler" FORCE)
elseif(DEFINED ENV{SY_CC})
    set(CMAKE_C_COMPILER $ENV{SY_CC} CACHE STRING "C compiler" FORCE)
else()
    # Auto-detect clang or GCC
    find_program(CLANG_PATH clang++)
    find_program(GCC_PATH g++)

    if(CLANG_PATH)
        message(STATUS "Using detected clang++ at ${CLANG_PATH}")
        set(CMAKE_CXX_COMPILER ${CLANG_PATH} CACHE STRING "C++ compiler" FORCE)
        set(CMAKE_C_COMPILER clang CACHE STRING "C compiler" FORCE)
    elseif(GCC_PATH)
        message(STATUS "Using detected g++ at ${GCC_PATH}")
        set(CMAKE_CXX_COMPILER ${GCC_PATH} CACHE STRING "C++ compiler" FORCE)
        set(CMAKE_C_COMPILER gcc CACHE STRING "C compiler" FORCE)
    else()
        message(FATAL_ERROR "No suitable C++ compiler found. Please set SY_CXX or install clang/gcc.")
    endif()
endif()

project(syrpc VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
#Whether the C++ standard is a requirement. 
set(CMAKE_CXX_STANDARD_REQUIRED True)
# Set the path to the dependencies folder from the top level folder
set(DEPENDENCIES_DIR "${CMAKE_SOURCE_DIR}/dependencies")
# Note to self: include is where the dependency header files live 
# and lib is where the libraries are

# The name of the main object file that can be used to run this project
add_executable(${PROJECT_NAME})
#add_subdirectory(lib)
#add_subdirectory(include)
target_sources(${PROJECT_NAME}
  PRIVATE
    src/syrpc.cpp
)

#add_library()
#target_link_libraries()
# Additional compiler flags
target_compile_options(${PROJECT_NAME} 
    PRIVATE
    -Wall
    -Wno-deprecated
    -fdiagnostics-color=always
    -g
)