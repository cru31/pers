# Third-party integration tests

# Add WebGPU tests if available
if(TARGET pers_static)
    # WebGPU initialization test (basic, no GLFW required)
    add_subdirectory(webgpu/initialization)
    
    # WebGPU triangle tests (require GLFW)
    find_package(glfw3 CONFIG QUIET)
    if(glfw3_FOUND)
        # Triangle tests disabled temporarily - need to update to v0.19.4.1 API
        # add_subdirectory(webgpu/triangle)
        # add_subdirectory(webgpu/triangle2)
        message(STATUS "WebGPU graphical tests temporarily disabled (API update needed)")
    else()
        message(STATUS "WebGPU graphical tests disabled (GLFW not found)")
        message(STATUS "To enable: build with -DVCPKG_MANIFEST_FEATURES=\"tests\"")
    endif()
endif()