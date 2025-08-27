# Troubleshooting macOS CI Build Failures

## Issue: Dynamic Library Loading Failures on macOS

### Problem Description
When building the Pers Graphics Engine with wgpu-native on macOS CI runners, executables fail to find the `libwgpu_native.dylib` library at runtime, resulting in errors like:

```
dyld[3531]: Library not loaded: /Users/runner/work/pers/pers/build/pers/wgpu-native-src/target/release/deps/libwgpu_native.dylib
Referenced from: <4FC7C9C1-14A2-3A04-A30E-158DB536DC64> /Users/runner/work/pers/pers/build/bin/test_pers_static
Reason: tried: '/Users/runner/work/pers/pers/build/pers/wgpu-native-src/target/release/deps/libwgpu_native.dylib' (no such file or directory)
```

### Root Cause
When Rust's cargo builds wgpu-native, it creates a dylib with hardcoded absolute paths in its install name. These paths are specific to the build machine and won't work when the binary is moved or run on different machines.

### Solution Implemented

#### 1. CMake Module for macOS Dylib Management
Created `cmake/MacOSFixDylib.cmake` to handle dylib path fixing:

```cmake
function(fix_macos_dylib_references target_name)
    if(NOT APPLE)
        return()
    endif()
    
    # Get paths and detect build method
    # Set RPATH for the target
    # Fix hardcoded dylib paths if needed
endfunction()
```

#### 2. Automatic Path Correction
The build system now:
- Detects whether wgpu-native was built from source or downloaded as prebuilt
- Sets appropriate RPATH values for executables
- Uses `install_name_tool` to fix hardcoded paths in executables

#### 3. RPATH Strategy
Executables are configured with multiple RPATH entries:
- `@loader_path`: The directory containing the executable
- `@loader_path/../lib`: Standard library location relative to executable
- Absolute fallback path to wgpu-native runtime directory

## Debugging Steps

### 1. Verify Library Paths
Check what libraries an executable is trying to load:
```bash
otool -L path/to/executable
```

### 2. Check RPATH Settings
View RPATH entries in an executable:
```bash
otool -l path/to/executable | grep -A 2 LC_RPATH
```

### 3. Verify Library Install Name
Check the install name of a dylib:
```bash
otool -D path/to/library.dylib
```

### 4. Manual Fix (if needed)
Fix dylib references manually:
```bash
# Fix the dylib's own install name
install_name_tool -id "@rpath/libwgpu_native.dylib" path/to/libwgpu_native.dylib

# Fix references in executable
install_name_tool -change /old/absolute/path/libwgpu_native.dylib @rpath/libwgpu_native.dylib path/to/executable
```

## Architecture Mismatch Issues

### Problem
vcpkg may install wrong architecture libraries (x64 instead of arm64 or vice versa).

### Solution
Set vcpkg triplet explicitly:
```bash
# For Apple Silicon
export VCPKG_DEFAULT_TRIPLET=arm64-osx
export VCPKG_TARGET_TRIPLET=arm64-osx

# For Intel
export VCPKG_DEFAULT_TRIPLET=x64-osx
export VCPKG_TARGET_TRIPLET=x64-osx
```

### Verification
Check binary architecture:
```bash
file path/to/binary
lipo -info path/to/binary
```

## CI-Specific Considerations

### GitHub Actions macOS Runners
- Default runners may be x86_64 even on newer versions
- Always explicitly set architecture in CMake:
  ```yaml
  - name: Configure
    run: |
      cmake -B build \
        -DCMAKE_OSX_ARCHITECTURES=${{ matrix.arch }} \
        -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
  ```

### Build Matrix Example
```yaml
strategy:
  matrix:
    include:
      - os: macos-latest
        arch: x86_64
        triplet: x64-osx
      - os: macos-latest
        arch: arm64
        triplet: arm64-osx
```

## Common Error Messages and Solutions

### Error: "cmake was installed from the local/pinned tap"
**Solution**: Uninstall and reinstall cmake
```bash
brew uninstall cmake 2>/dev/null || true
brew install cmake
```

### Error: "Library not loaded: @rpath/libwgpu_native.dylib"
**Solution**: Ensure RPATH is set correctly
```bash
# Add RPATH to executable
install_name_tool -add_rpath /path/to/lib/directory path/to/executable
```

### Error: "Building for macOS, but linking in dylib built for macOS"
**Solution**: Architecture mismatch
- Check CMAKE_OSX_ARCHITECTURES matches vcpkg triplet
- Ensure all dependencies are built for same architecture

## Prevention Strategies

### 1. Use Prebuilt Binaries When Possible
```cmake
cmake -B build -DFORCE_WGPU_DOWNLOAD=ON
```

### 2. Consistent Build Environment
- Use same compiler and architecture throughout
- Set environment variables at start of CI job
- Use CMake presets for consistent configuration

### 3. Test Locally in Docker
```bash
# Use macOS CI environment locally (if available)
docker run --rm -it macos-ci-image:latest
```

## Testing Dynamic Linking

### Quick Test Script
```bash
#!/bin/bash
# test_dylib.sh
for exe in build/bin/test_*; do
    echo "Testing $exe..."
    if $exe; then
        echo "  ✓ Success"
    else
        echo "  ✗ Failed"
        otool -L "$exe" | head -10
    fi
done
```

### Comprehensive Verification
```bash
# Check all executables for missing libraries
find build -type f -perm +111 | while read exe; do
    if file "$exe" | grep -q "Mach-O"; then
        echo "Checking $exe..."
        if otool -L "$exe" | grep -q "not found"; then
            echo "  WARNING: Missing libraries detected"
            otool -L "$exe" | grep "not found"
        fi
    fi
done
```

## Resources
- [Dynamic Library Programming Topics (Apple)](https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/DynamicLibraries/)
- [CMake RPATH Documentation](https://cmake.org/cmake/help/latest/prop_tgt/INSTALL_RPATH.html)
- [install_name_tool man page](https://www.unix.com/man-page/osx/1/install_name_tool/)