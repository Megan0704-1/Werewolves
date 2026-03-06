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

add_executable(test_template ${CMAKE_CURRENT_SOURCE_DIR}/test_template.cpp)
target_link_libraries(test_template PUBLIC werewolf_template_backend GTest::gtest_main)

include(GoogleTest)

gtest_discover_tests(test_template)

