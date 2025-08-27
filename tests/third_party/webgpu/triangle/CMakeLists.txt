cmake_minimum_required(VERSION 3.20)
project(WebGPUTriangle)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include local helper functions
include(${CMAKE_CURRENT_LIST_DIR}/copy_runtime_dep.cmake)

# Check if building as part of main project
if(NOT TARGET pers_static)
    # For standalone builds, include the setup script
    set(STANDALONE_SETUP "${CMAKE_CURRENT_LIST_DIR}/standalone_setup.cmake")
    if(EXISTS "${STANDALONE_SETUP}")
        include(${STANDALONE_SETUP})
    else()
        message(FATAL_ERROR "This test must be built as part of the main project. Use -DBUILD_TESTS=ON from root.")
    endif()
endif()

# Check if GLFW is available (should be provided by vcpkg when BUILD_TESTS=ON)
find_package(glfw3 CONFIG)
if(NOT glfw3_FOUND)
    message(WARNING "GLFW not found - WebGPU triangle test will be skipped")
    message(STATUS "To enable this test, build with: -DVCPKG_MANIFEST_FEATURES=\"tests\" -DBUILD_TESTS=ON")
    return()
endif()

# Create triangle executable
add_executable(webgpu_triangle main.cpp)

# For macOS, compile as Objective-C++ to use Cocoa
if(APPLE)
    set_source_files_properties(main.cpp PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )
endif()

# Link libraries
target_link_libraries(webgpu_triangle PRIVATE 
    pers_static  # Includes WebGPU
    glfw
)

# Link macOS frameworks if needed
if(APPLE)
    target_link_libraries(webgpu_triangle PRIVATE
        "-framework Cocoa"
        "-framework QuartzCore"
    )
endif()

# Copy shader file to output directory
add_custom_command(TARGET webgpu_triangle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/shader.wgsl"
    $<TARGET_FILE_DIR:webgpu_triangle>
)

# Set output directory to match other tests
set_target_properties(webgpu_triangle PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy runtime dependencies
copy_runtime_dependencies(webgpu_triangle)

# Add to CTest
enable_testing()
add_test(NAME webgpu_triangle COMMAND webgpu_triangle)
set_tests_properties(webgpu_triangle PROPERTIES
    ENVIRONMENT "CI_SKIP=1"  # Skip in CI since it needs a display
)

# Print build information
message(STATUS "========================================")
message(STATUS "WebGPU Triangle Test")
message(STATUS "Using pers library and GLFW from vcpkg")
message(STATUS "========================================")