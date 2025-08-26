# Setup for standalone build of WebGPU initialization test
# This file is only used when building the test directly, not as part of main project

set(PERS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
set(PERS_BUILD_DIR "${PERS_ROOT}/build" CACHE PATH "Path to pers build directory")

# Find pers library
find_library(PERS_LIB 
    NAMES pers_static libpers_static pers_static.a
    PATHS "${PERS_BUILD_DIR}/lib/Debug" 
          "${PERS_BUILD_DIR}/lib/Release" 
          "${PERS_BUILD_DIR}/lib"
    NO_DEFAULT_PATH
)

if(NOT PERS_LIB)
    message(FATAL_ERROR "Could not find pers_static library. Build the main project first.")
endif()

# Find WebGPU library based on platform (v25+ has lib subdirectory)
if(WIN32)
    find_library(WGPU_LIB
        NAMES wgpu_native.dll.lib wgpu_native.lib
        PATHS "${PERS_BUILD_DIR}/pers/wgpu-native-prebuilt/lib" 
              "${PERS_BUILD_DIR}/pers/wgpu-native-build/target/release"
        NO_DEFAULT_PATH
    )
elseif(APPLE)
    find_library(WGPU_LIB
        NAMES wgpu_native libwgpu_native.dylib
        PATHS "${PERS_BUILD_DIR}/pers/wgpu-native-prebuilt/lib" 
              "${PERS_BUILD_DIR}/pers/wgpu-native-build/target/release"
        NO_DEFAULT_PATH
    )
else()  # Linux/Unix
    find_library(WGPU_LIB
        NAMES wgpu_native libwgpu_native.so
        PATHS "${PERS_BUILD_DIR}/pers/wgpu-native-prebuilt/lib" 
              "${PERS_BUILD_DIR}/pers/wgpu-native-build/target/release"
        NO_DEFAULT_PATH
    )
endif()

if(NOT WGPU_LIB)
    message(FATAL_ERROR "Could not find wgpu_native library.")
endif()

# Add include directories
include_directories("${PERS_ROOT}/pers/include")
include_directories("${PERS_ROOT}/third_party/glm")
include_directories("${PERS_BUILD_DIR}/pers/wgpu-native-prebuilt/include/webgpu")

# Create imported target
add_library(pers_static STATIC IMPORTED)
set_target_properties(pers_static PROPERTIES
    IMPORTED_LOCATION "${PERS_LIB}"
    INTERFACE_LINK_LIBRARIES "${WGPU_LIB}"
)

message(STATUS "Standalone build configured:")
message(STATUS "  pers_static: ${PERS_LIB}")
message(STATUS "  wgpu_native: ${WGPU_LIB}")