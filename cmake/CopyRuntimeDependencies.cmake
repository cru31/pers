# Function to copy runtime dependencies to target directory
function(copy_runtime_dependencies target_name)
    # WebGPU runtime library
    if(EXISTS "${CMAKE_BINARY_DIR}/pers/wgpu-native-prebuilt")
        set(WGPU_PREBUILT_DIR "${CMAKE_BINARY_DIR}/pers/wgpu-native-prebuilt")
    elseif(EXISTS "${CMAKE_BINARY_DIR}/pers/wgpu-native-build/target/release")
        set(WGPU_PREBUILT_DIR "${CMAKE_BINARY_DIR}/pers/wgpu-native-build/target/release")
    else()
        return()
    endif()
    
    if(WIN32)
        # Windows: Copy .dll
        if(EXISTS "${WGPU_PREBUILT_DIR}/wgpu_native.dll")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_PREBUILT_DIR}/wgpu_native.dll"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying wgpu_native.dll for ${target_name}"
            )
        endif()
    elseif(APPLE)
        # macOS: Copy .dylib
        if(EXISTS "${WGPU_PREBUILT_DIR}/libwgpu_native.dylib")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_PREBUILT_DIR}/libwgpu_native.dylib"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying libwgpu_native.dylib for ${target_name}"
            )
        endif()
    elseif(UNIX)
        # Linux: Copy .so
        if(EXISTS "${WGPU_PREBUILT_DIR}/libwgpu_native.so")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_PREBUILT_DIR}/libwgpu_native.so"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying libwgpu_native.so for ${target_name}"
            )
        endif()
    endif()
endfunction()