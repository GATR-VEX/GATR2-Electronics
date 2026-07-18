cmake_minimum_required(VERSION 3.16)
project(locagatr_board_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# System gtest when present, fetched pinned copy otherwise.
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Test files, one folder per part as under src/.
set(TEST_SOURCES
    brainlink/wire_gtest.cpp
    brainlink/responder_gtest.cpp
    records/encoder_gtest.cpp
    sensor_acquisition/sensor_acquisition_gtest.cpp
)

# Sources under test.
set(CODE_UNDER_TEST
    ../src/brainlink/wire.cpp
    ../src/brainlink/responder.cpp
    ../src/records/encoder.cpp
    ../src/sensor_acquisition/sensor_acquisition.cpp
)

add_executable(locagatr_board_tests
    ${TEST_SOURCES}
    ${CODE_UNDER_TEST}
)
target_include_directories(locagatr_board_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_compile_options(locagatr_board_tests PRIVATE -Wall -Wextra -Werror)
target_link_libraries(locagatr_board_tests PRIVATE GTest::gtest_main)

enable_testing()
include(GoogleTest)
gtest_discover_tests(locagatr_board_tests)
