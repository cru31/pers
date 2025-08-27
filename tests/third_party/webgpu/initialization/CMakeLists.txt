cmake_minimum_required(VERSION 3.20)
project(WebGPUInitTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include local helper functions
include(${CMAKE_CURRENT_LIST_DIR}/copy_runtime_dep.cmake)

# Find pers library from the main project
if(NOT TARGET pers_static)
    # Support standalone build (e.g., in CLion)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/standalone_setup.cmake")
        include(${CMAKE_CURRENT_LIST_DIR}/standalone_setup.cmake)
    else()
        message(FATAL_ERROR "This test must be built as part of the main project")
    endif()
endif()

# Create test executable
add_executable(webgpu_init_test main.cpp)

# Link with pers library (which includes WebGPU)
target_link_libraries(webgpu_init_test PRIVATE pers_static)

# Set output directory
set_target_properties(webgpu_init_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Fix macOS dylib references (only on macOS)
if(APPLE)
    fix_macos_dylib_for_targets(webgpu_init_test)
endif()

# Copy runtime dependencies
copy_runtime_dependencies(webgpu_init_test)

# Add to CTest
enable_testing()
add_test(NAME webgpu_initialization COMMAND webgpu_init_test)

# Print build information
message(STATUS "========================================")
message(STATUS "WebGPU Initialization Test")
message(STATUS "Using pers library from main project")
message(STATUS "========================================")