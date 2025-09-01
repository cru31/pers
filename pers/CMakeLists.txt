cmake_minimum_required(VERSION 3.20)
if(APPLE)
    project(pers VERSION 0.1.0 LANGUAGES CXX OBJCXX)
else()
    project(pers VERSION 0.1.0 LANGUAGES CXX)
endif()

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Force UTF-8 encoding on MSVC
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Define WebGPU version
set(WGPU_VERSION "v25.0.2.1" CACHE STRING "wgpu-native version to use")
message(STATUS "Using wgpu-native version: ${WGPU_VERSION}")

# GLM is required for pers engine
find_package(glm CONFIG REQUIRED)
message(STATUS "Found GLM")

# GLFW is NOT needed for pers library itself
# It will be needed for tests/samples

# Options for WebGPU build method
option(FORCE_WGPU_DOWNLOAD "Force download of pre-built wgpu-native binaries instead of compiling" OFF)

# Check for Rust compiler (for wgpu-native)
if(NOT FORCE_WGPU_DOWNLOAD)
    execute_process(
        COMMAND rustc --version
        OUTPUT_VARIABLE RUST_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE RUST_RESULT
        ERROR_QUIET
    )
    if(RUST_RESULT EQUAL 0)
        message(STATUS "Found Rust: ${RUST_VERSION}")
        set(RUST_AVAILABLE TRUE)
    else()
        # Try with full path if rustc is not in PATH
        execute_process(
            COMMAND C:/Users/quake/.cargo/bin/rustc --version
            OUTPUT_VARIABLE RUST_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE RUST_RESULT
            ERROR_QUIET
        )
        if(RUST_RESULT EQUAL 0)
            message(STATUS "Found Rust: ${RUST_VERSION}")
            set(RUST_AVAILABLE TRUE)
            set(ENV{PATH} "$ENV{PATH};C:/Users/quake/.cargo/bin")
        else()
            message(WARNING "Rust compiler not found. Will check for pre-built wgpu-native.")
            set(RUST_AVAILABLE FALSE)
        endif()
    endif()
else()
    message(STATUS "FORCE_WGPU_DOWNLOAD is ON, skipping Rust detection")
    set(RUST_AVAILABLE FALSE)
endif()

# Setup wgpu-native
set(WGPU_NATIVE_AVAILABLE FALSE)
# Use third_party directory for persistent storage of dependencies
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# Define the runtime directory based on build method
set(WGPU_RUNTIME_BASE_DIR "${PROJECT_ROOT}/third_party/wgpu-native-runtime")

# Determine which subdirectory to use based on options
# macOS always uses prebuilt to avoid CI issues
if(APPLE)
    set(WGPU_BUILD_METHOD "prebuilt")
    message(STATUS "WebGPU build method: Pre-built binaries (macOS always uses prebuilt)")
elseif(RUST_AVAILABLE AND NOT FORCE_WGPU_DOWNLOAD)
    set(WGPU_BUILD_METHOD "custom-build")
    message(STATUS "WebGPU build method: Custom build from source")
else()
    set(WGPU_BUILD_METHOD "prebuilt")
    message(STATUS "WebGPU build method: Pre-built binaries")
endif()

set(WGPU_RUNTIME_DIR "${WGPU_RUNTIME_BASE_DIR}/${WGPU_BUILD_METHOD}")
file(MAKE_DIRECTORY "${WGPU_RUNTIME_DIR}")
file(MAKE_DIRECTORY "${WGPU_RUNTIME_DIR}/include")
file(MAKE_DIRECTORY "${WGPU_RUNTIME_DIR}/lib")

# Check if wgpu-native is already available in the runtime directory
if(WIN32)
    set(WGPU_RUNTIME_LIB "${WGPU_RUNTIME_DIR}/lib/wgpu_native.dll.lib")
    set(WGPU_RUNTIME_DLL "${WGPU_RUNTIME_DIR}/lib/wgpu_native.dll")
elseif(APPLE)
    set(WGPU_RUNTIME_LIB "${WGPU_RUNTIME_DIR}/lib/libwgpu_native.dylib")
else()
    set(WGPU_RUNTIME_LIB "${WGPU_RUNTIME_DIR}/lib/libwgpu_native.so")
endif()

set(WGPU_RUNTIME_HEADER "${WGPU_RUNTIME_DIR}/include/webgpu/wgpu.h")

# macOS ALWAYS downloads - no checks, no compile
if(APPLE)
    # Skip ALL checks for macOS - just download
    # Download pre-built binaries
    message(STATUS "Downloading pre-built wgpu-native binaries...")
    
    include(FetchContent)
    
    # Download to a temporary location
    set(WGPU_DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/wgpu-native-download")
    
    # Check architecture for macOS
    if(CMAKE_OSX_ARCHITECTURES)
        if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
            set(WGPU_PLATFORM "macos-aarch64")
        elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
            set(WGPU_PLATFORM "macos-x86_64")
        else()
            message(WARNING "Unknown macOS architecture: ${CMAKE_OSX_ARCHITECTURES}, defaulting to x86_64")
            set(WGPU_PLATFORM "macos-x86_64")
        endif()
    else()
        # Auto-detect architecture if not specified
        execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE DETECTED_ARCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(DETECTED_ARCH STREQUAL "arm64")
            set(WGPU_PLATFORM "macos-aarch64")
            set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architecture" FORCE)
        else()
            set(WGPU_PLATFORM "macos-x86_64")
            set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "macOS architecture" FORCE)
        endif()
        message(STATUS "Auto-detected macOS architecture: ${CMAKE_OSX_ARCHITECTURES}")
    endif()
    
    # Use the version defined at the top
    set(WGPU_URL "https://github.com/gfx-rs/wgpu-native/releases/download/${WGPU_VERSION}/wgpu-${WGPU_PLATFORM}-release.zip")
    
    FetchContent_Declare(
        wgpu_native_download
        URL ${WGPU_URL}
        SOURCE_DIR ${WGPU_DOWNLOAD_DIR}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    
    FetchContent_MakeAvailable(wgpu_native_download)
    
    # Copy downloaded files to the runtime directory
    message(STATUS "Installing wgpu-native to runtime directory...")
    
    # Copy all headers (preserving webgpu directory structure)
    file(COPY "${WGPU_DOWNLOAD_DIR}/include/" 
         DESTINATION "${WGPU_RUNTIME_DIR}/include")
    
    # Copy libraries
    file(GLOB WGPU_LIBS "${WGPU_DOWNLOAD_DIR}/lib/*")
    file(COPY ${WGPU_LIBS} DESTINATION "${WGPU_RUNTIME_DIR}/lib")
    
    # Verify installation
    if(NOT EXISTS "${WGPU_RUNTIME_HEADER}")
        message(FATAL_ERROR "Failed to install wgpu-native header")
    endif()
    if(NOT EXISTS "${WGPU_RUNTIME_LIB}")
        message(FATAL_ERROR "Failed to install wgpu-native library")
    endif()
    
    message(STATUS "Successfully installed wgpu-native to ${WGPU_RUNTIME_DIR}")
    set(WGPU_NATIVE_LIB "${WGPU_RUNTIME_LIB}")
    set(WGPU_NATIVE_INCLUDE_DIR "${WGPU_RUNTIME_DIR}/include")
    if(WIN32)
        set(WGPU_NATIVE_DLL "${WGPU_RUNTIME_DLL}" CACHE INTERNAL "Path to wgpu_native.dll")
    endif()
    set(WGPU_NATIVE_AVAILABLE TRUE)
elseif(NOT RUST_AVAILABLE OR FORCE_WGPU_DOWNLOAD)
    # Download pre-built binaries for Linux/Windows
    message(STATUS "Downloading pre-built wgpu-native binaries...")
    
    include(FetchContent)
    
    # Download to a temporary location
    set(WGPU_DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/wgpu-native-download")
    
    # Determine platform and architecture
    if(WIN32)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(WGPU_PLATFORM "windows-x86_64-msvc")
        else()
            message(FATAL_ERROR "32-bit Windows is not supported")
        endif()
    else()
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(WGPU_PLATFORM "linux-x86_64")
        else()
            message(FATAL_ERROR "32-bit Linux is not supported")
        endif()
    endif()
    
    # Use the version defined at the top
    set(WGPU_URL "https://github.com/gfx-rs/wgpu-native/releases/download/${WGPU_VERSION}/wgpu-${WGPU_PLATFORM}-release.zip")

    FetchContent_Declare(
            wgpu_native_download
            URL ${WGPU_URL}
            SOURCE_DIR ${WGPU_DOWNLOAD_DIR}
            # Fix CMake CMP0135 warning: Use extraction time for downloaded archive timestamps
            # This ensures proper rebuild detection when URL content changes
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(wgpu_native_download)
    
    # Copy downloaded files to the runtime directory
    message(STATUS "Installing wgpu-native to runtime directory...")
    
    # Copy all headers (preserving webgpu directory structure)
    file(COPY "${WGPU_DOWNLOAD_DIR}/include/" 
         DESTINATION "${WGPU_RUNTIME_DIR}/include")
    
    # Copy libraries
    file(GLOB WGPU_LIBS "${WGPU_DOWNLOAD_DIR}/lib/*")
    file(COPY ${WGPU_LIBS} DESTINATION "${WGPU_RUNTIME_DIR}/lib")
    
    # Verify installation
    if(NOT EXISTS "${WGPU_RUNTIME_HEADER}")
        message(FATAL_ERROR "Failed to install wgpu-native header")
    endif()
    if(NOT EXISTS "${WGPU_RUNTIME_LIB}")
        message(FATAL_ERROR "Failed to install wgpu-native library")
    endif()
    
    message(STATUS "Successfully installed wgpu-native to ${WGPU_RUNTIME_DIR}")
    set(WGPU_NATIVE_LIB "${WGPU_RUNTIME_LIB}")
    set(WGPU_NATIVE_INCLUDE_DIR "${WGPU_RUNTIME_DIR}/include")
    if(WIN32)
        set(WGPU_NATIVE_DLL "${WGPU_RUNTIME_DLL}" CACHE INTERNAL "Path to wgpu_native.dll")
    endif()
    set(WGPU_NATIVE_AVAILABLE TRUE)
elseif(RUST_AVAILABLE AND NOT FORCE_WGPU_DOWNLOAD)
    # If Rust is available and not forced to download, build from source
    message(STATUS "Building wgpu-native from source...")
    include(ExternalProject)
    
    set(WGPU_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/wgpu-native-src)
    set(WGPU_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/wgpu-native-build)
    
    # Set cargo path
    if(WIN32 AND EXISTS "C:/Users/quake/.cargo/bin/cargo.exe")
        set(CARGO_EXECUTABLE "C:/Users/quake/.cargo/bin/cargo.exe")
    else()
        set(CARGO_EXECUTABLE "cargo")
    endif()
    
    ExternalProject_Add(
        wgpu-native-build
        GIT_REPOSITORY https://github.com/gfx-rs/wgpu-native
        GIT_TAG ${WGPU_VERSION}
        SOURCE_DIR ${WGPU_SOURCE_DIR}
        BINARY_DIR ${WGPU_BUILD_DIR}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${CMAKE_COMMAND} -E env CARGO_TARGET_DIR=${WGPU_SOURCE_DIR}/target ${CARGO_EXECUTABLE} build --release --manifest-path ${WGPU_SOURCE_DIR}/Cargo.toml
        BUILD_IN_SOURCE 0
        INSTALL_COMMAND ""
        BUILD_ALWAYS 0
    )
    
    # After build, copy to runtime directory
    if(WIN32)
        set(WGPU_BUILD_LIB ${WGPU_SOURCE_DIR}/target/release/wgpu_native.dll.lib)
        set(WGPU_BUILD_DLL ${WGPU_SOURCE_DIR}/target/release/wgpu_native.dll)
    elseif(APPLE)
        set(WGPU_BUILD_LIB ${WGPU_SOURCE_DIR}/target/release/libwgpu_native.dylib)
    else()
        set(WGPU_BUILD_LIB ${WGPU_SOURCE_DIR}/target/release/libwgpu_native.so)
    endif()
    
    # Add a custom command to copy after build
    add_custom_command(
        OUTPUT "${WGPU_RUNTIME_LIB}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${WGPU_RUNTIME_DIR}/include/webgpu"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_SOURCE_DIR}/ffi/wgpu.h"
                "${WGPU_RUNTIME_DIR}/include/webgpu/wgpu.h"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_SOURCE_DIR}/ffi/webgpu-headers/webgpu.h"
                "${WGPU_RUNTIME_DIR}/include/webgpu/webgpu.h"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_BUILD_LIB}"
                "${WGPU_RUNTIME_LIB}"
        COMMAND ${CMAKE_COMMAND} -E echo "Installed wgpu-native to runtime directory"
        DEPENDS wgpu-native-build
    )
    
    # macOS: Fix install name for the copied dylib
    if(APPLE)
        add_custom_command(
            TARGET wgpu-native-build POST_BUILD
            COMMAND install_name_tool -id "@rpath/libwgpu_native.dylib" "${WGPU_RUNTIME_LIB}"
            COMMAND install_name_tool -change "${WGPU_SOURCE_DIR}/target/release/deps/libwgpu_native.dylib" "@rpath/libwgpu_native.dylib" "${WGPU_RUNTIME_LIB}"
            COMMENT "Fixing wgpu-native dylib install name"
        )
    endif()
    
    if(WIN32)
        add_custom_command(
            OUTPUT "${WGPU_RUNTIME_DLL}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${WGPU_BUILD_DLL}"
                    "${WGPU_RUNTIME_DLL}"
            DEPENDS wgpu-native-build
        )
        add_custom_target(wgpu-native-install ALL DEPENDS "${WGPU_RUNTIME_LIB}" "${WGPU_RUNTIME_DLL}")
    else()
        add_custom_target(wgpu-native-install ALL DEPENDS "${WGPU_RUNTIME_LIB}")
    endif()
    
    set(WGPU_NATIVE_LIB "${WGPU_RUNTIME_LIB}")
    set(WGPU_NATIVE_INCLUDE_DIR "${WGPU_RUNTIME_DIR}/include")
    if(WIN32)
        set(WGPU_NATIVE_DLL "${WGPU_RUNTIME_DLL}" CACHE INTERNAL "Path to wgpu_native.dll")
    endif()
    set(WGPU_NATIVE_AVAILABLE TRUE)
endif()

# Check if wgpu-native is available
if(NOT WGPU_NATIVE_AVAILABLE)
    message(FATAL_ERROR "Failed to configure wgpu-native. Please install Rust or ensure network access for downloading pre-built binaries.")
endif()

# Pers Engine Library Sources
file(GLOB_RECURSE PERS_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
)

# Add macOS-specific sources
if(APPLE)
    file(GLOB_RECURSE PERS_SOURCES_OBJC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm
    )
    list(APPEND PERS_SOURCES ${PERS_SOURCES_OBJC})
endif()

file(GLOB_RECURSE PERS_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)

# Create static library
add_library(pers_static STATIC ${PERS_SOURCES} ${PERS_HEADERS})
target_include_directories(pers_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(pers_static PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src  # For internal headers
)

# Link required dependencies
target_link_libraries(pers_static PUBLIC glm::glm)
target_include_directories(pers_static PUBLIC ${WGPU_NATIVE_INCLUDE_DIR})
target_link_libraries(pers_static PUBLIC ${WGPU_NATIVE_LIB})

# Link macOS frameworks if needed
if(APPLE)
    target_link_libraries(pers_static PUBLIC 
        "-framework Cocoa"
        "-framework QuartzCore"
        "-framework Metal"
    )
endif()

# Add dependency on wgpu-native build target if building from source
if(TARGET wgpu-native-install)
    add_dependencies(pers_static wgpu-native-install)
endif()

set_target_properties(pers_static PROPERTIES OUTPUT_NAME "pers_static")

# Create shared library
add_library(pers_shared SHARED ${PERS_SOURCES} ${PERS_HEADERS})
target_include_directories(pers_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(pers_shared PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src  # For internal headers
)

# Link required dependencies
target_link_libraries(pers_shared PUBLIC glm::glm)
target_include_directories(pers_shared PUBLIC ${WGPU_NATIVE_INCLUDE_DIR})
target_link_libraries(pers_shared PUBLIC ${WGPU_NATIVE_LIB})

# Link macOS frameworks if needed
if(APPLE)
    target_link_libraries(pers_shared PUBLIC 
        "-framework Cocoa"
        "-framework QuartzCore"
        "-framework Metal"
    )
endif()

# Add dependency on wgpu-native build target if building from source
if(TARGET wgpu-native-install)
    add_dependencies(pers_shared wgpu-native-install)
endif()

set_target_properties(pers_shared PROPERTIES 
    OUTPUT_NAME "pers"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
if(WIN32)
    target_compile_definitions(pers_shared PRIVATE PERS_BUILD_DLL)
    target_compile_definitions(pers_shared INTERFACE PERS_USE_DLL)
endif()

# Copy wgpu_native.dll to output directory for Windows
if(WIN32 AND WGPU_NATIVE_DLL)
    add_custom_command(TARGET pers_shared POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${WGPU_NATIVE_DLL}
        $<TARGET_FILE_DIR:pers_shared>
        COMMENT "Copying wgpu_native.dll to output directory"
    )
endif()

# Fix macOS dylib references for both libraries (only on macOS)
if(APPLE)
    fix_macos_dylib_for_targets(pers_static pers_shared)
endif()

# Export configuration
message(STATUS "===========================================")
message(STATUS "Pers Library Configuration")
message(STATUS "===========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "GLM: Required (math library)")
message(STATUS "WebGPU: Required (wgpu-native ${WGPU_VERSION})")
if(WGPU_NATIVE_PREBUILT_USED)
    message(STATUS "  Using pre-built binary")
else()
    message(STATUS "  Building from source")
endif()
message(STATUS "===========================================")