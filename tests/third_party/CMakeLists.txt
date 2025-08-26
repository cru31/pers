# Third-party integration tests

# Add WebGPU tests if available
if(TARGET pers_static)
    # WebGPU initialization test (basic, no GLFW required)
    add_subdirectory(webgpu/initialization)
    
    # WebGPU triangle tests (require GLFW)
    find_package(glfw3 CONFIG QUIET)
    if(glfw3_FOUND)
        add_subdirectory(webgpu/triangle)
        add_subdirectory(webgpu/triangle2)
        message(STATUS "WebGPU graphical tests enabled (GLFW found)")
    else()
        message(STATUS "WebGPU graphical tests disabled (GLFW not found)")
        message(STATUS "To enable: build with -DVCPKG_MANIFEST_FEATURES=\"tests\"")
    endif()
endif()