cmake_minimum_required(VERSION 3.20)

# Force Xcode generator on macOS
if(APPLE AND NOT CMAKE_GENERATOR)
    set(CMAKE_GENERATOR "Xcode" CACHE STRING "CMake generator" FORCE)
    message(STATUS "Forcing Xcode generator on macOS")
endif()

project(pers_graphics_engine VERSION 0.1.0 LANGUAGES CXX)

# Force vcpkg manifest mode
set(VCPKG_MANIFEST_MODE ON CACHE BOOL "Use vcpkg manifest mode" FORCE)
set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "vcpkg manifest directory" FORCE)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options for building tests and samples
option(BUILD_TESTS "Build test programs" ON)
option(BUILD_SAMPLES "Build sample programs" OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include macOS-specific dylib fix module
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/MacOSFixDylib.cmake)

# Add pers library subdirectory
add_subdirectory(pers)

# Debug vcpkg triplet on macOS
if(APPLE)
    if(DEFINED VCPKG_TARGET_TRIPLET)
        message(STATUS "Using vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")
    endif()
    if(DEFINED CMAKE_OSX_ARCHITECTURES)
        message(STATUS "Building for macOS architecture: ${CMAKE_OSX_ARCHITECTURES}")
    endif()
endif()

# Check if GLFW is needed and available
if(BUILD_TESTS OR BUILD_SAMPLES)
    find_package(glfw3 CONFIG)
    if(NOT glfw3_FOUND)
        message(WARNING "GLFW not found. Tests and samples that require GLFW will be disabled.")
        message(STATUS "To enable GLFW support, install vcpkg features:")
        message(STATUS "  vcpkg install --feature=tests")
        message(STATUS "  vcpkg install --feature=samples")
    endif()
endif()

# Enable testing if requested
if(BUILD_TESTS)
    enable_testing()
    # Add tests subdirectory
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
        add_subdirectory(tests)
    endif()
endif()

# Add samples if requested
if(BUILD_SAMPLES)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/samples)
        add_subdirectory(samples)
    endif()
endif()

# Print main configuration
message(STATUS "===========================================")
message(STATUS "Pers Graphics Engine Main Configuration")
message(STATUS "===========================================")
message(STATUS "Project Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "===========================================")