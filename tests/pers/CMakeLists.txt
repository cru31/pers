cmake_minimum_required(VERSION 3.20)

# Test for pers static library
add_executable(test_pers_static test_static.cpp)
target_link_libraries(test_pers_static PRIVATE pers_static)

# Test for pers shared library
add_executable(test_pers_shared test_shared.cpp)
target_link_libraries(test_pers_shared PRIVATE pers_shared)

# Add tests
enable_testing()
add_test(NAME test_static COMMAND test_pers_static)
add_test(NAME test_shared COMMAND test_pers_shared)