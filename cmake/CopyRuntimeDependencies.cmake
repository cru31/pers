# Copy runtime dependencies for executables
# This is a centralized function to copy runtime dependencies to the target's output directory

function(copy_runtime_dependencies target_name)
    # Get project root
    get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." ABSOLUTE)
    
    # Define the single runtime directory
    set(WGPU_RUNTIME_DIR "${PROJECT_ROOT}/third_party/wgpu-native-runtime/lib")
    
    if(WIN32)
        # Copy wgpu_native.dll
        if(EXISTS "${WGPU_RUNTIME_DIR}/wgpu_native.dll")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_RUNTIME_DIR}/wgpu_native.dll"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying wgpu_native.dll for ${target_name}"
            )
        endif()
        
        # Copy GLFW DLL if it exists (vcpkg may build it as shared)
        if(EXISTS "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/glfw3.dll")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/glfw3.dll"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying glfw3.dll for ${target_name}"
            )
        endif()
        
    elseif(APPLE)
        # Copy libwgpu_native.dylib
        if(EXISTS "${WGPU_RUNTIME_DIR}/libwgpu_native.dylib")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_RUNTIME_DIR}/libwgpu_native.dylib"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying libwgpu_native.dylib for ${target_name}"
            )
        endif()
        
        # Copy GLFW library if it exists
        if(EXISTS "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-osx/lib/libglfw.3.dylib")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-osx/lib/libglfw.3.dylib"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying libglfw.3.dylib for ${target_name}"
            )
        endif()
        
    elseif(UNIX)
        # Copy libwgpu_native.so
        if(EXISTS "${WGPU_RUNTIME_DIR}/libwgpu_native.so")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_RUNTIME_DIR}/libwgpu_native.so"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying libwgpu_native.so for ${target_name}"
            )
        endif()
        
        # Copy GLFW library if it exists
        if(EXISTS "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libglfw.so.3")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/libglfw.so.3"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying libglfw.so.3 for ${target_name}"
            )
        endif()
    endif()
endfunction()