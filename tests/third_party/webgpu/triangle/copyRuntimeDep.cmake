# Copy runtime dependencies for webgpu_triangle
function(copy_runtime_dependencies target_name)
    # WebGPU runtime library
    if(EXISTS "${CMAKE_BINARY_DIR}/pers/wgpu-native-prebuilt")
        set(WGPU_DIR "${CMAKE_BINARY_DIR}/pers/wgpu-native-prebuilt")
    elseif(EXISTS "${CMAKE_BINARY_DIR}/pers/wgpu-native-build/target/release")
        set(WGPU_DIR "${CMAKE_BINARY_DIR}/pers/wgpu-native-build/target/release")
    else()
        return()
    endif()
    
    if(WIN32)
        # Copy wgpu_native.dll
        if(EXISTS "${WGPU_DIR}/wgpu_native.dll")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_DIR}/wgpu_native.dll"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying wgpu_native.dll for ${target_name}"
            )
        endif()
        
        # GLFW DLL might be needed if using shared libs (though vcpkg usually builds static)
        # Add here if needed in the future
        
    elseif(APPLE)
        if(EXISTS "${WGPU_DIR}/libwgpu_native.dylib")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_DIR}/libwgpu_native.dylib"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying libwgpu_native.dylib for ${target_name}"
            )
        endif()
    elseif(UNIX)
        if(EXISTS "${WGPU_DIR}/libwgpu_native.so")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_DIR}/libwgpu_native.so"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying libwgpu_native.so for ${target_name}"
            )
        endif()
    endif()
endfunction()