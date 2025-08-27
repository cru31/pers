# Third-Party Library Integration Guide

## Overview
This document provides comprehensive guidelines for integrating third-party libraries into the Pers Graphics Engine project. We handle two distinct categories of dependencies:

1. **vcpkg-supported libraries** - Available through vcpkg package manager
2. **Non-vcpkg libraries** - Must be manually integrated (e.g., wgpu-native)

## Directory Structure

```
pers_graphics_engine/
├── third_party/                  # Non-vcpkg libraries
│   ├── wgpu-native-runtime/     # Persistent WebGPU binaries
│   │   ├── prebuilt/            # Downloaded pre-built binaries
│   │   └── custom-build/        # Locally compiled binaries
│   └── [other-libraries]/       # Other non-vcpkg dependencies
├── vcpkg.json                    # vcpkg manifest for dependencies
├── vcpkg-configuration.json     # vcpkg configuration
└── CMakeLists.txt               # Main build configuration
```

## 1. Adding vcpkg-Supported Libraries

### Step 1: Check Library Availability
```bash
# Search for library in vcpkg registry
vcpkg search <library-name>
```

### Step 2: Add to vcpkg Manifest
Edit `vcpkg.json`:
```json
{
  "dependencies": [
    "glm",
    "glfw3",
    "your-new-library"  // Add here
  ],
  "features": {
    "tests": {
      "description": "Build test suite",
      "dependencies": [
        "glfw3",
        "your-test-library"  // Or add to test features
      ]
    }
  }
}
```

### Step 3: Update CMakeLists.txt
```cmake
# In pers/CMakeLists.txt or appropriate subdirectory
find_package(your-library CONFIG REQUIRED)

# Link to target
target_link_libraries(pers_static PUBLIC 
    your-library::your-library  # or appropriate target name
)
```

### Step 4: Platform-Specific Considerations
```cmake
# Handle platform differences if needed
if(WIN32)
    # Windows-specific configuration
elseif(APPLE)
    # macOS-specific configuration
elseif(UNIX)
    # Linux-specific configuration
endif()
```

### Step 5: CI/CD Integration
The CI workflow automatically handles vcpkg dependencies, but verify:
- Library works on all target platforms (Windows, Linux, macOS)
- Consider architecture differences (x64, ARM64)

## 2. Adding Non-vcpkg Libraries

### Step 1: Determine Integration Strategy

Choose one of three approaches:

#### Option A: Pre-built Binaries (Recommended for stable releases)
- Download official releases
- Store in `third_party/<library-name>-runtime/prebuilt/`
- Best for: Libraries with regular releases (e.g., wgpu-native)

#### Option B: Source Compilation
- Clone and build from source
- Store build output in `third_party/<library-name>-runtime/custom-build/`
- Best for: Libraries requiring custom patches or specific versions

#### Option C: Git Submodule (Header-only or small libraries)
- Add as git submodule
- Best for: Header-only libraries or libraries requiring source access

### Step 2: Implement Download/Build Logic

Example from wgpu-native integration:

```cmake
# Define version
set(LIBRARY_VERSION "v1.0.0" CACHE STRING "Library version to use")

# Setup runtime directory
set(LIBRARY_RUNTIME_BASE_DIR "${PROJECT_ROOT}/third_party/library-runtime")

# Option to force download or compile
option(FORCE_LIBRARY_DOWNLOAD "Force download pre-built binaries" OFF)
option(FORCE_LIBRARY_COMPILE "Force compilation from source" OFF)

# Check if library is already available
if(EXISTS "${LIBRARY_RUNTIME_DIR}/lib/library${CMAKE_SHARED_LIBRARY_SUFFIX}")
    # Use existing installation
elseif(FORCE_LIBRARY_DOWNLOAD OR NOT COMPILER_AVAILABLE)
    # Download pre-built binaries
    include(FetchContent)
    FetchContent_Declare(
        library_download
        URL https://github.com/org/library/releases/download/${LIBRARY_VERSION}/library-${PLATFORM}.zip
        SOURCE_DIR ${DOWNLOAD_DIR}
    )
    FetchContent_MakeAvailable(library_download)
    
    # Copy to runtime directory
    file(COPY "${DOWNLOAD_DIR}/include/" DESTINATION "${RUNTIME_DIR}/include")
    file(COPY "${DOWNLOAD_DIR}/lib/" DESTINATION "${RUNTIME_DIR}/lib")
else()
    # Build from source
    include(ExternalProject)
    ExternalProject_Add(
        library_build
        GIT_REPOSITORY https://github.com/org/library
        GIT_TAG ${LIBRARY_VERSION}
        BUILD_COMMAND <appropriate-build-command>
        INSTALL_COMMAND ""
    )
endif()
```

### Step 3: Create CMake Helper Module

Create `cmake/FindLibrary.cmake` or `cmake/LibraryHelpers.cmake`:

```cmake
# Helper function to setup the library
function(setup_library_dependency)
    # Implementation details
endfunction()
```

### Step 4: Handle Runtime Dependencies

For Windows DLLs:
```cmake
# Copy DLL to output directory
if(WIN32 AND LIBRARY_DLL)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${LIBRARY_DLL}
        $<TARGET_FILE_DIR:${TARGET_NAME}>
    )
endif()
```

### Step 5: Add to .gitignore

```gitignore
# Ignore downloaded/built dependencies
third_party/*/
!third_party/*.cmake
!third_party/README.md
```

## 3. Testing Integration

### Local Testing Checklist

- [ ] Clean build succeeds
- [ ] Tests pass with new library
- [ ] No missing runtime dependencies
- [ ] Works in both Debug and Release modes

### CI/CD Verification

- [ ] All platform builds succeed (Windows, Linux, macOS)
- [ ] Architecture variants work (x64, ARM64)
- [ ] Cache keys properly configured
- [ ] Artifacts include necessary files

## 4. Best Practices

### Version Management
- **Always pin versions** - Never use "latest" or floating versions
- **Use CMake cache variables** - Allow version override via command line
- **Document version requirements** - Include minimum/maximum supported versions

### Platform Compatibility
- **Test all platforms** - Don't assume cross-platform compatibility
- **Handle architecture differences** - ARM64 vs x64 especially on macOS
- **Consider compiler differences** - MSVC, GCC, Clang

### Performance Considerations
- **Prefer static linking** - When possible, to avoid runtime dependency issues
- **Minimize download size** - Only download necessary components
- **Cache aggressively** - Both locally and in CI

### Security
- **Verify checksums** - When downloading pre-built binaries
- **Use HTTPS** - Never download over insecure connections
- **Review licenses** - Ensure compatibility with project license

## 5. Common Patterns

### Pattern 1: Dual Strategy (Download vs Compile)
Used for: wgpu-native
```cmake
if(RUST_AVAILABLE AND NOT FORCE_DOWNLOAD)
    # Compile from source
else()
    # Download pre-built
endif()
```

### Pattern 2: Feature-Gated Dependencies
Used for: Test-only dependencies
```json
{
  "features": {
    "tests": {
      "dependencies": ["test-framework"]
    }
  }
}
```

### Pattern 3: Platform-Specific Dependencies
```json
{
  "dependencies": [
    {
      "name": "library",
      "platform": "!windows"
    }
  ]
}
```

## 6. Build Order and Dependencies

### Critical: ExternalProject Build Order Issues

When using `ExternalProject_Add` to build libraries from source, you MUST explicitly declare dependencies:

```cmake
# Example from wgpu-native integration
ExternalProject_Add(wgpu-native-build ...)

# Create install target that depends on build completion
add_custom_target(wgpu-native-install ALL DEPENDS "${LIBRARY_OUTPUT}")

# CRITICAL: Add explicit dependencies for all consuming targets
add_library(your_library ...)
if(TARGET wgpu-native-install)
    add_dependencies(your_library wgpu-native-install)
endif()
```

**Why This Is Critical:**
- **macOS/Xcode**: Aggressively parallelizes all targets, causing link failures if library isn't built yet
- **Windows/Linux**: Handle dependencies better but still need explicit ordering for correctness
- **Symptom**: "no such file or directory" errors during linking despite successful configuration

**Real-world Example:**
```cmake
# pers/CMakeLists.txt
target_link_libraries(pers_static PUBLIC ${WGPU_NATIVE_LIB})

# Without this, macOS builds fail with missing libwgpu_native.dylib
if(TARGET wgpu-native-install)
    add_dependencies(pers_static wgpu-native-install)
endif()
```

## 7. Troubleshooting

### Common Issues and Solutions

#### Issue: vcpkg triplet mismatch
**Solution**: Ensure CMAKE_OSX_ARCHITECTURES and VCPKG_TARGET_TRIPLET align
```cmake
-DVCPKG_TARGET_TRIPLET=arm64-osx
-DCMAKE_OSX_ARCHITECTURES=arm64
```

#### Issue: Missing runtime dependencies
**Solution**: Add post-build copy commands for DLLs/dylibs

#### Issue: Header not found
**Solution**: Verify include directories are properly set
```cmake
target_include_directories(target PUBLIC ${LIBRARY_INCLUDE_DIR})
```

#### Issue: Link errors
**Solution**: Check library naming conventions per platform
- Windows: `.lib` (import) + `.dll` (runtime)
- Linux: `.so` or `.a`
- macOS: `.dylib` or `.a`

## 7. Example Integrations

### Example 1: Adding Dear ImGui (vcpkg)
```json
// vcpkg.json
{
  "dependencies": [
    "imgui[core,glfw-binding,opengl3-binding]"
  ]
}
```

```cmake
# CMakeLists.txt
find_package(imgui CONFIG REQUIRED)
target_link_libraries(app PRIVATE imgui::imgui)
```

### Example 2: Adding Custom Physics Engine (non-vcpkg)
```cmake
# third_party/physics/CMakeLists.txt
include(FetchContent)
FetchContent_Declare(
    physics_engine
    GIT_REPOSITORY https://github.com/org/physics
    GIT_TAG v2.1.0
)
FetchContent_MakeAvailable(physics_engine)
```

## 8. Maintenance

### Regular Updates
- Review dependency versions monthly
- Test updates in feature branches
- Update CI caches after major changes

### Documentation
- Document why each dependency is needed
- Keep integration examples updated
- Note any workarounds or special cases

### Cleanup
- Remove unused dependencies
- Clean old cached versions periodically
- Audit third_party directory size

## Summary

1. **Prefer vcpkg** when available - simpler integration and maintenance
2. **Use persistent directories** for non-vcpkg libraries - avoid re-downloading
3. **Support multiple strategies** - download vs compile based on environment
4. **Test thoroughly** - all platforms, architectures, and configurations
5. **Document everything** - future maintainers will thank you

## References

- [vcpkg Documentation](https://vcpkg.io/en/docs/README.html)
- [CMake FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)
- [CMake ExternalProject](https://cmake.org/cmake/help/latest/module/ExternalProject.html)