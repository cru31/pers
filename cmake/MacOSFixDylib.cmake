# MacOSFixDylib.cmake
# macOS-specific dynamic library path fixing for wgpu-native
# This file handles the complexities of macOS dylib loading without affecting other platforms

if(NOT APPLE)
    return()
endif()

# Function to fix wgpu-native dylib references in executables
function(fix_macos_dylib_references target_name)
    if(NOT APPLE)
        return()
    endif()
    
    # Get project root and determine paths
    get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}" ABSOLUTE)
    set(WGPU_RUNTIME_DIR "${PROJECT_ROOT}/third_party/wgpu-native-runtime")
    
    # Check which build method was used (custom-build or prebuilt)
    if(EXISTS "${WGPU_RUNTIME_DIR}/custom-build/lib/libwgpu_native.dylib")
        set(WGPU_LIB_DIR "${WGPU_RUNTIME_DIR}/custom-build/lib")
        # Path where cargo builds the dylib with wrong install name
        set(OLD_DYLIB_PATH "${CMAKE_BINARY_DIR}/pers/wgpu-native-src/target/release/deps/libwgpu_native.dylib")
        set(NEEDS_FIX TRUE)
    elseif(EXISTS "${WGPU_RUNTIME_DIR}/prebuilt/lib/libwgpu_native.dylib")
        set(WGPU_LIB_DIR "${WGPU_RUNTIME_DIR}/prebuilt/lib")
        set(NEEDS_FIX FALSE)  # Prebuilt binaries don't have this issue
    else()
        message(WARNING "wgpu-native dylib not found for ${target_name}")
        return()
    endif()
    
    # Get target type
    get_target_property(target_type ${target_name} TYPE)
    
    # Skip static libraries - they don't need dylib path fixing
    if(target_type STREQUAL "STATIC_LIBRARY")
        return()
    endif()
    
    # Set RPATH for the target (only for executables and shared libraries)
    set_target_properties(${target_name} PROPERTIES
        BUILD_RPATH "${WGPU_LIB_DIR}"
        INSTALL_RPATH "@loader_path;@loader_path/../lib;${WGPU_LIB_DIR}"
    )
    
    # Fix hardcoded dylib path after build (only needed for custom-build)
    if(NEEDS_FIX)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND install_name_tool -change 
                "${OLD_DYLIB_PATH}" 
                "@rpath/libwgpu_native.dylib" 
                "$<TARGET_FILE:${target_name}>"
            COMMENT "Fixing wgpu-native dylib path in ${target_name}"
            VERBATIM
        )
    endif()
endfunction()

# Function to setup dylib for a list of targets
function(fix_macos_dylib_for_targets)
    if(NOT APPLE)
        return()
    endif()
    
    foreach(target ${ARGN})
        if(TARGET ${target})
            fix_macos_dylib_references(${target})
        endif()
    endforeach()
endfunction()

# Export the runtime directory for child projects
if(APPLE)
    get_filename_component(PROJECT_ROOT "${CMAKE_SOURCE_DIR}" ABSOLUTE)
    set(WGPU_RUNTIME_DIR "${PROJECT_ROOT}/third_party/wgpu-native-runtime" CACHE PATH "WebGPU runtime directory" FORCE)
endif()