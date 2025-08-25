cmake_minimum_required(VERSION 3.20)
project(WebGPUTriangle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific variables
if(WIN32)
    set(PLATFORM_NAME "windows")
    set(LIB_PREFIX "")
    set(LIB_EXTENSION ".dll")
    set(IMPLIB_EXTENSION ".dll.lib")
    set(NEEDS_IMPLIB TRUE)
    set(COPY_LIBRARY TRUE)
elseif(APPLE)
    set(PLATFORM_NAME "macos")
    set(LIB_PREFIX "lib")
    set(LIB_EXTENSION ".dylib")
    set(NEEDS_IMPLIB FALSE)
    set(COPY_LIBRARY TRUE)
    set(CMAKE_MACOSX_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "@loader_path")
elseif(UNIX)
    set(PLATFORM_NAME "linux")
    set(LIB_PREFIX "lib")
    set(LIB_EXTENSION ".so")
    set(NEEDS_IMPLIB FALSE)
    set(COPY_LIBRARY FALSE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

# Common paths
set(WGPU_ROOT "${CMAKE_SOURCE_DIR}/../../../../third_party/wgpu-native")
set(WGPU_INCLUDE_DIR "${WGPU_ROOT}/ffi")
set(WGPU_LIB_DIR "${WGPU_ROOT}/target/release")
set(WGPU_LIB_FILE "${WGPU_LIB_DIR}/${LIB_PREFIX}wgpu_native${LIB_EXTENSION}")

# Find GLFW
set(GLFW_ROOT "${CMAKE_SOURCE_DIR}/../../../../third_party/glfw")
if(NOT EXISTS "${GLFW_ROOT}")
    # Try to find system GLFW
    find_package(glfw3 3.3 REQUIRED)
else()
    # Use local GLFW
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${GLFW_ROOT} glfw_build)
endif()

# Add wgpu-native library
add_library(wgpu_native SHARED IMPORTED)
set_target_properties(wgpu_native PROPERTIES
    IMPORTED_LOCATION "${WGPU_LIB_FILE}"
    INTERFACE_INCLUDE_DIRECTORIES "${WGPU_INCLUDE_DIR};${WGPU_INCLUDE_DIR}/webgpu-headers"
)

# Add import library for Windows
if(NEEDS_IMPLIB)
    set_target_properties(wgpu_native PROPERTIES
        IMPORTED_IMPLIB "${WGPU_LIB_DIR}/wgpu_native${IMPLIB_EXTENSION}"
    )
endif()

# Create triangle executable
add_executable(webgpu_triangle main.cpp)

# Link libraries
if(TARGET glfw)
    target_link_libraries(webgpu_triangle PRIVATE wgpu_native glfw)
else()
    target_link_libraries(webgpu_triangle PRIVATE wgpu_native glfw3::glfw)
endif()

# Copy shader file to output directory
add_custom_command(TARGET webgpu_triangle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/shader.wgsl"
    $<TARGET_FILE_DIR:webgpu_triangle>
)

# Copy library to output directory if needed
if(COPY_LIBRARY)
    add_custom_command(TARGET webgpu_triangle POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WGPU_LIB_FILE}"
        $<TARGET_FILE_DIR:webgpu_triangle>
    )
endif()

# Print build information
message(STATUS "========================================")
message(STATUS "Platform: ${PLATFORM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "wgpu-native library: ${WGPU_LIB_FILE}")
message(STATUS "========================================")