# Prefer an installed GoogleTest; download it only when none is found
find_package(GTest QUIET)
if (NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif ()

include(GoogleTest)

add_executable(test_Interrupt test_Interrupt.cpp)
target_link_libraries(test_Interrupt PRIVATE InterruptLib GTest::gtest_main)
gtest_discover_tests(test_Interrupt)
