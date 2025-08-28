# Third-party integration tests

# Add WebGPU tests if available
if(TARGET pers_static)
    # WebGPU initialization test (basic, no GLFW required)
    add_subdirectory(webgpu/initialization)
    
    # WebGPU triangle tests (require GLFW)
    find_package(glfw3 CONFIG QUIET)
    if(glfw3_FOUND)
        add_subdirectory(webgpu/triangle)
        message(STATUS "WebGPU graphical tests enabled")
    else()
        message(STATUS "WebGPU graphical tests disabled (GLFW not found)")
        message(STATUS "To enable: build with -DVCPKG_MANIFEST_FEATURES=\"tests\"")
    endif()
endif()

# Add SDL3 test
find_package(SDL3 CONFIG QUIET)
if(SDL3_FOUND)
    add_subdirectory(sdl)
    message(STATUS "SDL3 test enabled")
else()
    message(STATUS "SDL3 test disabled (SDL3 not found)")
endif()

# Add Assimp test  
find_package(assimp CONFIG QUIET)
if(assimp_FOUND)
    add_subdirectory(assimp)
    message(STATUS "Assimp test enabled")
else()
    message(STATUS "Assimp test disabled (Assimp not found)")
endif()