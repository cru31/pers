# Copy runtime dependencies for webgpu_triangle
function(copy_runtime_dependencies target_name)
    # WebGPU runtime library - check multiple possible locations (v25 has lib subdirectory)
    set(POSSIBLE_WGPU_DIRS
        "${CMAKE_BINARY_DIR}/pers/wgpu-native-prebuilt/lib"
        "${CMAKE_BINARY_DIR}/pers/wgpu-native-build/target/release"
        "${CMAKE_CURRENT_LIST_DIR}/../../../../build/pers/wgpu-native-prebuilt/lib"
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
        
        # Copy GLFW DLL if it exists (vcpkg may build it as shared)
        set(POSSIBLE_GLFW_DIRS
            "${CMAKE_CURRENT_LIST_DIR}/../../../../build/vcpkg_installed/x64-windows/bin"
            "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin"
        )
        
        foreach(DIR ${POSSIBLE_GLFW_DIRS})
            if(EXISTS "${DIR}/glfw3.dll")
                add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DIR}/glfw3.dll"
                    $<TARGET_FILE_DIR:${target_name}>
                    COMMENT "Copying glfw3.dll for ${target_name}"
                )
                break()
            endif()
        endforeach()
        
    elseif(APPLE)
        if(EXISTS "${WGPU_DIR}/libwgpu_native.dylib")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_DIR}/libwgpu_native.dylib"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying libwgpu_native.dylib for ${target_name}"
            )
        endif()
        
        # Copy GLFW library if it exists (vcpkg may build it as shared)
        set(POSSIBLE_GLFW_DIRS
            "${CMAKE_CURRENT_LIST_DIR}/../../../../build/vcpkg_installed/x64-osx/lib"
            "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-osx/lib"
        )
        
        foreach(DIR ${POSSIBLE_GLFW_DIRS})
            if(EXISTS "${DIR}/libglfw.3.dylib")
                add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DIR}/libglfw.3.dylib"
                    $<TARGET_FILE_DIR:${target_name}>
                    COMMENT "Copying libglfw.3.dylib for ${target_name}"
                )
                break()
            endif()
        endforeach()
        
    elseif(UNIX)
        if(EXISTS "${WGPU_DIR}/libwgpu_native.so")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_DIR}/libwgpu_native.so"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying libwgpu_native.so for ${target_name}"
            )
        endif()
        
        # Copy GLFW library if it exists (vcpkg may build it as shared)
        set(POSSIBLE_GLFW_DIRS
            "${CMAKE_CURRENT_LIST_DIR}/../../../../build/vcpkg_installed/x64-linux/lib"
            "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib"
        )
        
        foreach(DIR ${POSSIBLE_GLFW_DIRS})
            if(EXISTS "${DIR}/libglfw.so.3")
                add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DIR}/libglfw.so.3"
                    $<TARGET_FILE_DIR:${target_name}>
                    COMMENT "Copying libglfw.so.3 for ${target_name}"
                )
                break()
            endif()
        endforeach()
    endif()
endfunction()