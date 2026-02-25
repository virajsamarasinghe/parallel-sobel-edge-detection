cmake_minimum_required(VERSION 3.10)
project(SobelEdgeDetection CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimization flags for baseline
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra")
endif()

# Include directories
include_directories(include)

# Executable
add_executable(sobel_serial
    src/main.cpp
    src/sobel.cpp
    src/utils.cpp
)
