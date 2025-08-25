cmake_minimum_required(VERSION 3.20)

project(WebGPUTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force Clang compiler if available
if(NOT DEFINED CMAKE_C_COMPILER AND NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(CLANG_C_COMPILER NAMES clang clang-cl)
    find_program(CLANG_CXX_COMPILER NAMES clang++ clang-cl)
    if(CLANG_C_COMPILER AND CLANG_CXX_COMPILER)
        set(CMAKE_C_COMPILER ${CLANG_C_COMPILER})
        set(CMAKE_CXX_COMPILER ${CLANG_CXX_COMPILER})
        message(STATUS "Using Clang: ${CMAKE_CXX_COMPILER}")
    endif()
endif()

# Platform-specific variables
if(WIN32)
    set(PLATFORM_NAME "windows")
    set(LIB_PREFIX "")
    set(LIB_EXTENSION ".dll")
    set(IMPLIB_EXTENSION ".dll.lib")
    set(NEEDS_IMPLIB TRUE)
    set(COPY_LIBRARY TRUE)
    
    # Windows specific compiler flags for Clang-CL compatibility
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-fms-compatibility -fms-extensions)
    endif()
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
    set(COPY_LIBRARY FALSE)  # Linux typically uses rpath instead of copying
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

# Common paths
set(WGPU_ROOT "${CMAKE_SOURCE_DIR}/../../../../third_party/wgpu-native")
set(WGPU_INCLUDE_DIR "${WGPU_ROOT}/ffi")
set(WGPU_LIB_DIR "${WGPU_ROOT}/target/release")
set(WGPU_LIB_FILE "${WGPU_LIB_DIR}/${LIB_PREFIX}wgpu_native${LIB_EXTENSION}")

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

# Create test executable
add_executable(webgpu_test main.cpp)
target_link_libraries(webgpu_test PRIVATE wgpu_native)

# Copy library to output directory if needed
if(COPY_LIBRARY)
    add_custom_command(TARGET webgpu_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WGPU_LIB_FILE}"
        $<TARGET_FILE_DIR:webgpu_test>
    )
endif()

# Print build information
message(STATUS "========================================")
message(STATUS "Platform: ${PLATFORM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "wgpu-native library: ${WGPU_LIB_FILE}")
message(STATUS "========================================")