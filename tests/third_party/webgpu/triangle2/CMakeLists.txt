cmake_minimum_required(VERSION 3.20)
project(WebGPUTriangle2)

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

# Check if GLFW is available (should be provided by vcpkg when BUILD_TESTS=ON)
find_package(glfw3 CONFIG)
if(NOT glfw3_FOUND)
    message(WARNING "GLFW not found - WebGPU triangle2 test will be skipped")
    message(STATUS "To enable this test, build with: -DVCPKG_MANIFEST_FEATURES=\"tests\" -DBUILD_TESTS=ON")
    return()
endif()

# Create triangle2 executable
add_executable(webgpu_triangle2 main.cpp)

# Link libraries
target_link_libraries(webgpu_triangle2 PRIVATE 
    pers_static  # Includes WebGPU
    glfw
)

# Copy shader file to output directory
add_custom_command(TARGET webgpu_triangle2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/shader.wgsl"
    $<TARGET_FILE_DIR:webgpu_triangle2>
)

# Set output directory to match other tests
set_target_properties(webgpu_triangle2 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy runtime dependencies
copy_runtime_dependencies(webgpu_triangle2)

# Add to CTest
enable_testing()
add_test(NAME webgpu_triangle2 COMMAND webgpu_triangle2)
set_tests_properties(webgpu_triangle2 PROPERTIES
    ENVIRONMENT "CI_SKIP=1"  # Skip in CI since it needs a display
)

# Print build information
message(STATUS "========================================")
message(STATUS "WebGPU Triangle2 Test")
message(STATUS "Using pers library and GLFW from vcpkg")
message(STATUS "========================================")