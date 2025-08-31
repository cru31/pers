cmake_minimum_required(VERSION 3.20)
project(PersTriangle)

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
    message(WARNING "GLFW not found - Pers triangle test will be skipped")
    message(STATUS "To enable this test, build with: -DVCPKG_MANIFEST_FEATURES=\"tests\" -DBUILD_TESTS=ON")
    return()
endif()

# Create triangle executable
set(TRIANGLE_SOURCES
    main.cpp
    PersTriangleApp.cpp
    PersTriangleApp.h
    TriangleRenderer.cpp
    TriangleRenderer.h
    GLFWWindow.h
    GLFWWindow.cpp
    GLFWWindowFactory.h
    GLFWWindowFactory.cpp
)

# Add macOS-specific sources
if(APPLE)
    list(APPEND TRIANGLE_SOURCES TriangleRenderer_macOS.mm)
endif()

add_executable(pers_triangle ${TRIANGLE_SOURCES})

# For macOS, compile as Objective-C++ to use Cocoa
if(APPLE)
    set_source_files_properties(main.cpp PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )
endif()

# Link libraries
target_link_libraries(pers_triangle PRIVATE 
    pers_static  # Includes WebGPU
    glfw
)

# Link macOS frameworks if needed
if(APPLE)
    target_link_libraries(pers_triangle PRIVATE
        "-framework Cocoa"
        "-framework QuartzCore"
        "-framework Metal"
    )
endif()

# Set output directory to match other tests
set_target_properties(pers_triangle PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Fix macOS dylib references (only on macOS)
if(APPLE)
    fix_macos_dylib_for_targets(pers_triangle)
endif()

# Copy shader file to output directory
add_custom_command(TARGET pers_triangle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/shader.wgsl"
    $<TARGET_FILE_DIR:pers_triangle>
)

# Copy runtime dependencies
copy_runtime_dependencies(pers_triangle)

# Add to CTest
# IMPORTANT: This test MUST be run with PERS_TEST_MODE=1 environment variable
# Otherwise it will run indefinitely as a graphical application
# The test mode ensures automatic exit after 5 seconds to prevent timeout in CI/test environments
enable_testing()
add_test(NAME pers_triangle COMMAND pers_triangle)
set_tests_properties(pers_triangle PROPERTIES
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:pers_triangle>"
    ENVIRONMENT "PERS_TEST_MODE=1"  # REQUIRED: Enables auto-exit after 5 seconds
)

# Note: This test is excluded from CI runs via --exclude-regex in workflow files
# because it requires GPU/display for WebGPU surface creation

# Print build information
message(STATUS "========================================")
message(STATUS "Pers Triangle Test")
message(STATUS "Using pers library and GLFW from vcpkg")
message(STATUS "========================================")