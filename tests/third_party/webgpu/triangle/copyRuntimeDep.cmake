# Copy runtime dependencies for webgpu_triangle
function(copy_runtime_dependencies target_name)
    # WebGPU runtime library - check multiple possible locations
    set(POSSIBLE_WGPU_DIRS
        "${CMAKE_BINARY_DIR}/pers/wgpu-native-prebuilt"
        "${CMAKE_BINARY_DIR}/pers/wgpu-native-build/target/release"
        "${CMAKE_CURRENT_LIST_DIR}/../../../../build/pers/wgpu-native-prebuilt"
        "${CMAKE_CURRENT_LIST_DIR}/../../../../build/pers/wgpu-native-build/target/release"
    )
    
    set(WGPU_DIR "")
    foreach(DIR ${POSSIBLE_WGPU_DIRS})
        if(EXISTS "${DIR}")
            set(WGPU_DIR "${DIR}")
            break()
        endif()
    endforeach()
    
    if(WGPU_DIR STREQUAL "")
        message(WARNING "Could not find WebGPU runtime libraries to copy")
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