# frontend server libraries 
add_library(werewolf_server_shm_lib STATIC ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp)
target_link_libraries(werewolf_server_shm_lib PRIVATE werewolf_core)

# frontend client libraries 
add_library(werewolf_client_shm_lib STATIC ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp)
target_link_libraries(werewolf_client_shm_lib PRIVATE werewolf_core)

# shm server and shm
add_executable(werewolf_shm_server ${CMAKE_CURRENT_SOURCE_DIR}/shm_server.cpp)
target_link_libraries(werewolf_shm_server PRIVATE werewolf_server_shm_lib werewolf_shm_backend)

add_executable(werewolf_shm_client ${CMAKE_CURRENT_SOURCE_DIR}/shm_client.cpp)
target_link_libraries(werewolf_shm_client PRIVATE werewolf_client_shm_lib werewolf_shm_backend)
