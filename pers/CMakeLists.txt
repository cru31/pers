cmake_minimum_required(VERSION 3.20)
project(pers VERSION 0.1.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# GLM is required for pers engine
find_package(glm CONFIG REQUIRED)
message(STATUS "Found GLM")

# GLFW is NOT needed for pers library itself
# It will be needed for tests/samples

# Check for Rust compiler (for wgpu-native)
execute_process(
    COMMAND rustc --version
    OUTPUT_VARIABLE RUST_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE RUST_RESULT
)
if(RUST_RESULT EQUAL 0)
    message(STATUS "Found Rust: ${RUST_VERSION}")
    set(RUST_AVAILABLE TRUE)
else()
    message(WARNING "Rust compiler not found. Will check for pre-built wgpu-native.")
    set(RUST_AVAILABLE FALSE)
endif()

# Setup wgpu-native
set(WGPU_NATIVE_AVAILABLE FALSE)
set(WGPU_NATIVE_PREBUILT "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wgpu-native-bin")

# First check for pre-built binary
if(EXISTS "${WGPU_NATIVE_PREBUILT}/wgpu_native.dll.lib" AND EXISTS "${WGPU_NATIVE_PREBUILT}/wgpu.h")
    message(STATUS "Found pre-built wgpu-native at ${WGPU_NATIVE_PREBUILT}")
    set(WGPU_NATIVE_INCLUDE_DIR "${WGPU_NATIVE_PREBUILT}")
    set(WGPU_NATIVE_LIB "${WGPU_NATIVE_PREBUILT}/wgpu_native.dll.lib")
    set(WGPU_NATIVE_DLL "${WGPU_NATIVE_PREBUILT}/wgpu_native.dll")
    set(WGPU_NATIVE_AVAILABLE TRUE)
    set(WGPU_NATIVE_PREBUILT_USED TRUE)
elseif(RUST_AVAILABLE)
    # If Rust is available, build from source
    message(STATUS "Building wgpu-native from source...")
    include(ExternalProject)
    
    set(WGPU_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/wgpu-native-src)
    set(WGPU_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/wgpu-native-build)
    
    ExternalProject_Add(
        wgpu-native-build
        GIT_REPOSITORY https://github.com/gfx-rs/wgpu-native
        GIT_TAG v0.19.4.1
        SOURCE_DIR ${WGPU_SOURCE_DIR}
        BINARY_DIR ${WGPU_BUILD_DIR}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND cargo build --release --manifest-path ${WGPU_SOURCE_DIR}/Cargo.toml
        BUILD_IN_SOURCE 0
        INSTALL_COMMAND ""
    )
    
    # Set paths for the built library
    if(WIN32)
        set(WGPU_NATIVE_LIB ${WGPU_SOURCE_DIR}/target/release/wgpu_native.dll.lib)
        set(WGPU_NATIVE_DLL ${WGPU_SOURCE_DIR}/target/release/wgpu_native.dll)
    endif()
    set(WGPU_NATIVE_INCLUDE_DIR ${WGPU_SOURCE_DIR}/ffi)
    
    # Create imported target
    add_library(wgpu_native INTERFACE)
    target_link_libraries(wgpu_native INTERFACE ${WGPU_NATIVE_LIB})
    target_include_directories(wgpu_native INTERFACE ${WGPU_NATIVE_INCLUDE_DIR})
    add_dependencies(wgpu_native wgpu-native-build)
    
    set(WGPU_NATIVE_AVAILABLE TRUE)
    set(WGPU_NATIVE_PREBUILT_USED FALSE)
else()
    message(FATAL_ERROR "WebGPU support requires either:\n"
                        "1. Pre-built wgpu-native binaries in third_party/wgpu-native-bin/\n"
                        "2. Rust compiler installed from https://rustup.rs")
endif()

# Pers Engine Library Sources
file(GLOB_RECURSE PERS_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
)

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

# Link required dependencies
target_link_libraries(pers_static PUBLIC glm::glm)
if(WGPU_NATIVE_PREBUILT_USED)
    # Using pre-built binary
    target_include_directories(pers_static PUBLIC ${WGPU_NATIVE_INCLUDE_DIR})
    target_link_libraries(pers_static PUBLIC ${WGPU_NATIVE_LIB})
else()
    # Using source build
    target_link_libraries(pers_static PUBLIC wgpu_native)
endif()

set_target_properties(pers_static PROPERTIES OUTPUT_NAME "pers_static")

# Create shared library
add_library(pers_shared SHARED ${PERS_SOURCES} ${PERS_HEADERS})
target_include_directories(pers_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link required dependencies
target_link_libraries(pers_shared PUBLIC glm::glm)
if(WGPU_NATIVE_PREBUILT_USED)
    # Using pre-built binary
    target_include_directories(pers_shared PUBLIC ${WGPU_NATIVE_INCLUDE_DIR})
    target_link_libraries(pers_shared PUBLIC ${WGPU_NATIVE_LIB})
else()
    # Using source build
    target_link_libraries(pers_shared PUBLIC wgpu_native)
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

# Export configuration
message(STATUS "===========================================")
message(STATUS "Pers Library Configuration")
message(STATUS "===========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "GLM: Required (math library)")
message(STATUS "WebGPU: Required (wgpu-native v0.19.4.1)")
if(WGPU_NATIVE_PREBUILT_USED)
    message(STATUS "  Using pre-built binary")
else()
    message(STATUS "  Building from source")
endif()
message(STATUS "===========================================")