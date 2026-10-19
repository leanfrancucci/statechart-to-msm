cmake_minimum_required(VERSION 3.16)

project(Interrupt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(INTERRUPT_BUILD_TESTS "Build the Interrupt unit tests" ON)

# Boost.MSM is header-only
find_package(Boost 1.70 CONFIG REQUIRED)
find_package(Threads REQUIRED)

# State machine, active object and support code, shared by the interactive
# application and the unit tests
add_library(InterruptLib STATIC
    src/Log.cpp
    src/Interrupt.cpp
    src/InterruptActObj.cpp)
target_include_directories(InterruptLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(InterruptLib PUBLIC Boost::headers Threads::Threads)
# Boost.MPL caps its sequences at 20 elements by default, which a real
# transition table (mpl::vector) or event set (boost::variant, built on
# mpl::list) quickly exceeds. Raise the limits to 50, the most MPL supports.
target_compile_definitions(InterruptLib PUBLIC
    BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
    BOOST_MPL_LIMIT_VECTOR_SIZE=50
    BOOST_MPL_LIMIT_LIST_SIZE=50
    BOOST_MPL_LIMIT_MAP_SIZE=50)
target_compile_options(InterruptLib PUBLIC
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Werror>)

# Interactive application to stimulate the state machine from the keyboard
add_executable(Interrupt src/main.cpp)
target_link_libraries(Interrupt PRIVATE InterruptLib)

if (INTERRUPT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
