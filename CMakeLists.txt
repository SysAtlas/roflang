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
list(FILTER sources EXCLUDE REGEX ".*/roflang\\.cpp$")

add_library(roflang ${sources})
target_include_directories(roflang PUBLIC include)
target_compile_definitions(roflang PUBLIC ${LLVM_DEFINITIONS})
target_link_libraries(roflang PUBLIC LLVM)

add_executable(roflc src/roflang.cpp)
target_link_libraries(roflc PRIVATE roflang)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()