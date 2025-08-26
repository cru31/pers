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

# Rust is REQUIRED for wgpu-native
execute_process(
    COMMAND rustc --version
    OUTPUT_VARIABLE RUST_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE RUST_RESULT
)
if(NOT RUST_RESULT EQUAL 0)
    message(FATAL_ERROR "Rust compiler not found. Rust is required for WebGPU support.\nInstall from https://rustup.rs")
endif()
message(STATUS "Found Rust: ${RUST_VERSION}")

# wgpu-native is REQUIRED
include(FetchContent)

# Check for pre-built binary first
set(WGPU_NATIVE_PREBUILT "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/wgpu-native-bin")

if(EXISTS "${WGPU_NATIVE_PREBUILT}/wgpu_native.dll.lib")
    message(STATUS "Using pre-built wgpu-native from ${WGPU_NATIVE_PREBUILT}")
    set(WGPU_NATIVE_INCLUDE_DIR "${WGPU_NATIVE_PREBUILT}")
    set(WGPU_NATIVE_LIB "${WGPU_NATIVE_PREBUILT}/wgpu_native.dll.lib")
    set(WGPU_NATIVE_DLL "${WGPU_NATIVE_PREBUILT}/wgpu_native.dll")
    set(WGPU_NATIVE_PREBUILT_USED TRUE)
else()
    message(STATUS "Building wgpu-native from source...")
    FetchContent_Declare(
        wgpu-native
        GIT_REPOSITORY https://github.com/gfx-rs/wgpu-native
        GIT_TAG v0.19.4.1  # Fixed version for reproducibility
    )
    FetchContent_MakeAvailable(wgpu-native)
    set(WGPU_NATIVE_PREBUILT_USED FALSE)
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
if(WGPU_NATIVE_PREBUILT_USED)
    # Using pre-built binary
    target_include_directories(pers_static PUBLIC ${WGPU_NATIVE_INCLUDE_DIR})
    target_link_libraries(pers_static PUBLIC glm::glm ${WGPU_NATIVE_LIB})
else()
    # Using FetchContent build
    target_link_libraries(pers_static PUBLIC glm::glm wgpu_native)
endif()

set_target_properties(pers_static PROPERTIES OUTPUT_NAME "pers_static")

# Create shared library
add_library(pers_shared SHARED ${PERS_SOURCES} ${PERS_HEADERS})
target_include_directories(pers_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link required dependencies
if(WGPU_NATIVE_PREBUILT_USED)
    # Using pre-built binary
    target_include_directories(pers_shared PUBLIC ${WGPU_NATIVE_INCLUDE_DIR})
    target_link_libraries(pers_shared PUBLIC glm::glm ${WGPU_NATIVE_LIB})
else()
    # Using FetchContent build
    target_link_libraries(pers_shared PUBLIC glm::glm wgpu_native)
endif()

set_target_properties(pers_shared PROPERTIES 
    OUTPUT_NAME "pers"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
if(WIN32)
    target_compile_definitions(pers_shared PRIVATE PERS_BUILD_DLL)
    target_compile_definitions(pers_shared INTERFACE PERS_USE_DLL)
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