cmake_minimum_required(VERSION 3.7)
project(Boost_server_1)

find_package(Boost 1.63.0 COMPONENTS system filesystem thread REQUIRED)

set(CMAKE_CXX_STANDARD 14)
if(Boost_FOUND)
    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")

    include_directories(${Boost_INCLUDE_DIRS})


endif()
set(SOURCE_FILES main.cpp chat_message.h chat.h func.h func.cpp)
add_executable(Boost_server_1 ${SOURCE_FILES})
if(Boost_FOUND)
    target_link_libraries(Boost_server_1 ${Boost_LIBRARIES})
endif()