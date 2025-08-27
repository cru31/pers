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

# macOS-specific: Set RPATH for test executables to find wgpu-native dylib
if(APPLE)
    # Get the wgpu runtime directory from parent scope
    get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
    set(WGPU_RUNTIME_DIR "${PROJECT_ROOT}/third_party/wgpu-native-runtime")
    
    # Set RPATH for all test executables
    set_target_properties(test_pers_static test_pers_shared test_pers_webgpu PROPERTIES
        BUILD_RPATH "${WGPU_RUNTIME_DIR}/custom-build/lib;${WGPU_RUNTIME_DIR}/prebuilt/lib"
        INSTALL_RPATH "@loader_path;@loader_path/../lib;${WGPU_RUNTIME_DIR}/custom-build/lib;${WGPU_RUNTIME_DIR}/prebuilt/lib"
    )
endif()

# Add tests
enable_testing()
add_test(NAME test_static COMMAND test_pers_static)
add_test(NAME test_shared COMMAND test_pers_shared)
add_test(NAME test_webgpu COMMAND test_pers_webgpu)