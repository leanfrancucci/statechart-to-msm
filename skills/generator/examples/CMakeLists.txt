# include(CheckIncludeFile)

# CHECK_INCLUDE_FILE(/usr/include/linux/can/isotp.h IS_CANUTILS_ISOTP)
# if (NOT IS_CANUTILS_ISOTP)
    # message(STATUS "File isotp.h of can-utils does not found")
# endif()

#set(FOO_$<IF:$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,8>,release-1.12.1,rel> "")
#file(GENERATE OUTPUT filename CONTENT "$IF:$<CXX_COMPILER_VERSION:8>,true,false")

add_library(objlib${LEARNING_BOOST_NAME} OBJECT 
    ${CMAKE_CURRENT_SOURCE_DIR}/CompositeState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Mgr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EventQueue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SimpleState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Log.cpp)

add_executable(${LEARNING_BOOST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(objlib${LEARNING_BOOST_NAME} fmt::fmt)
target_link_libraries(${LEARNING_BOOST_NAME} objlib${LEARNING_BOOST_NAME})
target_compile_features(objlib${LEARNING_BOOST_NAME} PUBLIC cxx_std_11)
target_compile_options(objlib${LEARNING_BOOST_NAME} PRIVATE -Wall -Werror)
target_include_directories(objlib${LEARNING_BOOST_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)

include(boost)
include(fmt)
