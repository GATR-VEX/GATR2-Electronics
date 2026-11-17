# locaGATR Brain link: host build and tests. Frames and records come from the
# board firmware.

set(LOCAGATR_BOARD ${CMAKE_CURRENT_SOURCE_DIR}/../../pico/locaGATR)

add_library(locagatr STATIC
    src/link.cpp
    ${LOCAGATR_BOARD}/src/brainlink/wire.cpp
    ${LOCAGATR_BOARD}/src/records/encoder.cpp
)
target_include_directories(locagatr PUBLIC include ${LOCAGATR_BOARD}/include)
target_compile_features(locagatr PUBLIC cxx_std_17)
if(NOT MSVC)
    target_compile_options(locagatr PRIVATE -Wall -Wextra -Werror)
endif()

# Test files.
set(LOCAGATR_TEST_SOURCES
    tests/link_gtest.cpp
)

add_executable(locagatr_tests ${LOCAGATR_TEST_SOURCES})
target_link_libraries(locagatr_tests PRIVATE locagatr GTest::gtest_main)
if(NOT MSVC)
    target_compile_options(locagatr_tests PRIVATE -Wall -Wextra -Werror)
endif()
gtest_discover_tests(locagatr_tests)
