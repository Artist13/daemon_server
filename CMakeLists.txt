cmake_minimum_required(VERSION 3.10)

project(testIMA)

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
)

add_executable(daemon)
target_sources(
  daemon
  PUBLIC
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/server/daemon.cpp
)
