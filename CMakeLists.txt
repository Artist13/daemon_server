cmake_minimum_required(VERSION 3.10)

project(testIMA)

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Threads)

include_directories(./include ./src)

add_library(logger INTERFACE)
target_include_directories(logger INTERFACE include/logger/)

add_executable(client)
target_sources(
  client
  PUBLIC
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/client/client.cpp
)

add_library(server)
target_sources(
  server
  PUBLIC
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/server/server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/include/server/server.hpp
)
target_link_libraries(server PUBLIC ${CMAKE_THREAD_LIBS_INIT} INTERFACE logger)

add_executable(simple_server)
target_sources(
  simple_server
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/server/main.cpp
)
target_link_libraries(simple_server PUBLIC server)

add_executable(daemon)
target_sources(
  daemon
  PUBLIC
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/server/daemon.cpp
)
target_link_libraries(daemon PUBLIC server ${CMAKE_THREAD_LIBS_INIT} INTERFACE logger)

add_definitions(-DLOGGING_LEVEL_1)