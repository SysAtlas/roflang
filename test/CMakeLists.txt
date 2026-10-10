find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp"
)

add_executable(roflang_unit_tests ${test_sources})

target_include_directories(roflang_unit_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/support"
)

target_link_libraries(roflang_unit_tests PRIVATE
    roflang
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(roflang_unit_tests)