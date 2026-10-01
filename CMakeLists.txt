cmake_minimum_required(VERSION 4.4.3)
project(kaleidoscope)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

find_package(LLVM REQUIRED CONFIG)

file(GLOB_RECURSE sources src/*.cpp)

add_executable(toy ${sources})
target_include_directories(toy PRIVATE include)
target_compile_definitions(toy PRIVATE ${LLVM_DEFINITIONS})
target_link_libraries(toy PRIVATE LLVM-22)