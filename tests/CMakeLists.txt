# for gtest
include(FetchContent)

# download googletest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

# build gtest and add to project
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_template ${CMAKE_CURRENT_SOURCE_DIR}/template/test_template.cpp)
add_executable(test_pipe ${CMAKE_CURRENT_SOURCE_DIR}/pipe/test_pipe.cpp)

target_link_libraries(test_template PUBLIC werewolf_template_backend GTest::gtest_main)
target_link_libraries(test_pipe PUBLIC werewolf_pipe_backend GTest::gtest_main)

target_include_directories(test_template PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/utils)
target_include_directories(test_pipe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/utils)

include(GoogleTest)

gtest_discover_tests(test_template)
gtest_discover_tests(test_pipe)
