# Copy runtime dependencies for executables
# This is a centralized function to copy runtime dependencies to the target's output directory

function(copy_runtime_dependencies target_name)
    # Get project root
    get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." ABSOLUTE)
    
    if(WIN32)
        # Copy wgpu_native.dll using the CACHE variable from pers/CMakeLists.txt
        if(DEFINED WGPU_NATIVE_DLL AND EXISTS "${WGPU_NATIVE_DLL}")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WGPU_NATIVE_DLL}"
                $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying wgpu_native.dll for ${target_name}"
            )
        endif()
        
        # Find vcpkg installed directory (could be in different locations)
        if(EXISTS "${CMAKE_BINARY_DIR}/vcpkg_installed")
            set(VCPKG_INSTALL_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed")
        elseif(EXISTS "${_VCPKG_INSTALLED_DIR}")
            set(VCPKG_INSTALL_DIR "${_VCPKG_INSTALLED_DIR}")
        else()
            # Try to find it relative to the binary dir
            file(GLOB VCPKG_DIRS "${CMAKE_BINARY_DIR}/../*/vcpkg_installed")
            if(VCPKG_DIRS)
                list(GET VCPKG_DIRS 0 VCPKG_INSTALL_DIR)
            endif()
        endif()
        
        # Copy vcpkg DLLs if found
        if(VCPKG_INSTALL_DIR)
            # For Debug configuration, look in debug/bin first
            if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND EXISTS "${VCPKG_INSTALL_DIR}/x64-windows/debug/bin")
                file(GLOB DEBUG_DLLS "${VCPKG_INSTALL_DIR}/x64-windows/debug/bin/*.dll")
                foreach(DLL ${DEBUG_DLLS})
                    add_custom_command(TARGET ${target_name} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${DLL}"
                        $<TARGET_FILE_DIR:${target_name}>
                        COMMENT "Copying ${DLL} for ${target_name}"
                    )
                endforeach()
            else()
                # For Release or if debug/bin doesn't exist, use regular bin
                file(GLOB RELEASE_DLLS "${VCPKG_INSTALL_DIR}/x64-windows/bin/*.dll")
                foreach(DLL ${RELEASE_DLLS})
                    add_custom_command(TARGET ${target_name} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${DLL}"
                        $<TARGET_FILE_DIR:${target_name}>
                        COMMENT "Copying ${DLL} for ${target_name}"
                    )
                endforeach()
            endif()
        endif()
        
        # Also check for GLFW DLL specifically (fallback)
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