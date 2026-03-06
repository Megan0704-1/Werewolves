cmake_minimum_required(VERSION 3.14)

project(werewolf LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# core library, used by server and client
add_library(werewolf_core STATIC src/game.cpp)
target_include_directories(werewolf_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# load all backends
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/backends)

# apps
# add_executable(werewolf_server apps/server.cpp)
# add_executable(werewolf_client apps/client.cpp)
# 
# target_link_libraries(werewolf_server werewolf_core)
# target_link_libraries(werewolf_client werewolf_core)

# tests
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
