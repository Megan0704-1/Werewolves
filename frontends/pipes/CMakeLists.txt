# frontend server libraries 
add_library(werewolf_server STATIC ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp)
target_link_libraries(werewolf_server PRIVATE werewolf_core)

# frontend client libraries 
add_library(werewolf_client STATIC ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp)
target_link_libraries(werewolf_client PRIVATE werewolf_core)

# pipe server and pipe clients
add_executable(werewolf_pipe_server ${CMAKE_CURRENT_SOURCE_DIR}/pipe_server.cpp)
target_link_libraries(werewolf_pipe_server PRIVATE werewolf_server werewolf_pipe_backend)

add_executable(werewolf_pipe_client ${CMAKE_CURRENT_SOURCE_DIR}/pipe_client.cpp)
target_link_libraries(werewolf_pipe_client PRIVATE werewolf_client werewolf_pipe_backend)
