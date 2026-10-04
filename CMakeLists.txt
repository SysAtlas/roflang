cmake_minimum_required(VERSION 4.4.3)
project(roflang)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-fsanitize=address -Wall -Wextra -Wpedantic)
add_link_options(-fsanitize=address)

find_package(LLVM REQUIRED CONFIG)

file(GLOB_RECURSE sources src/*.cpp)

add_executable(roflang ${sources})
target_include_directories(roflang PRIVATE include)
target_compile_definitions(roflang PRIVATE ${LLVM_DEFINITIONS})
target_link_libraries(roflang PRIVATE LLVM)