cmake_minimum_required(VERSION 3.20)

# Test for pers static library
add_executable(test_pers_static test_static.cpp)
target_link_libraries(test_pers_static PRIVATE pers_static)

# Test for pers shared library
add_executable(test_pers_shared test_shared.cpp)
target_link_libraries(test_pers_shared PRIVATE pers_shared)

# Test for WebGPU integration
add_executable(test_pers_webgpu test_webgpu.cpp)
target_link_libraries(test_pers_webgpu PRIVATE pers_static)

# Fix macOS dylib references for all test executables (only on macOS)
if(APPLE)
    fix_macos_dylib_for_targets(test_pers_static test_pers_shared test_pers_webgpu)
endif()

# Add tests
enable_testing()
add_test(NAME test_static COMMAND test_pers_static)
add_test(NAME test_shared COMMAND test_pers_shared)
add_test(NAME test_webgpu COMMAND test_pers_webgpu)