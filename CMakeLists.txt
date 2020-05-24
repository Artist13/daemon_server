cmake_minimum_required(VERSION 3.10)

project(testIMA)

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Threads)

include_directories(./include ./src)

add_executable(client)
target_sources(
  client
  PUBLIC
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/client/client.cpp
)

add_executable(server)
target_sources(
  server
  PUBLIC
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/server/server.cpp
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/server/server.hpp
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/server/main.cpp
)
target_link_libraries (server ${CMAKE_THREAD_LIBS_INIT})

add_executable(daemon)
target_sources(
  daemon
  PUBLIC
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/server/daemon.cpp
)
