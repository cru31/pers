# Dependency Management Quick Reference

## Current Dependencies

### vcpkg-managed (vcpkg.json)
- **glm** - Math library (vectors, matrices)
- **glfw3** - Window management (test features only)

### Manually managed (third_party/)
- **wgpu-native** v25.0.2.1 - WebGPU implementation
  - Location: `third_party/wgpu-native-runtime/`
  - Strategy: Download pre-built OR compile from source

## Adding New Dependencies - Decision Tree

```
Is the library available in vcpkg?
├─ YES → Add to vcpkg.json
│   └─ Run: vcpkg install
└─ NO → Add to third_party/
    ├─ Has pre-built releases? → Use FetchContent to download
    ├─ Need custom build? → Use ExternalProject
    └─ Header-only? → Use git submodule
```

## Quick Commands

### Add vcpkg dependency
```bash
# 1. Search for library
vcpkg search <library>

# 2. Add to vcpkg.json
# Edit manually

# 3. Install locally
vcpkg install --triplet=x64-windows  # or your platform
```

### Add non-vcpkg dependency
```bash
# Create directory structure
mkdir -p third_party/<library>-runtime/{prebuilt,custom-build}

# Add CMake integration in pers/CMakeLists.txt
# Follow wgpu-native pattern
```

### Test new dependency
```bash
# Clean build test
rm -rf build
cmake --preset=windows-debug  # or your platform
cmake --build build --parallel

# Run tests
ctest --test-dir build
```

## Platform-Specific Triplets

| Platform | Architecture | vcpkg Triplet | CMAKE_OSX_ARCHITECTURES |
|----------|-------------|---------------|-------------------------|
| Windows | x64 | x64-windows | - |
| Linux | x64 | x64-linux | - |
| macOS | ARM64 (M1/M2) | arm64-osx | arm64 |
| macOS | Intel | x64-osx | x86_64 |

## Build Presets

### Windows
- `windows-debug` - Debug build with tests
- `windows-release` - Release build with tests
- `windows-debug-download` - Use pre-built WebGPU
- `windows-debug-compile` - Compile WebGPU from source

### Linux
- `linux-debug` - Debug build with tests
- `linux-release` - Release build with tests

### macOS
- `macos-debug` - ARM64 debug build
- `macos-release` - ARM64 release build
- `macos-x64-debug` - Intel debug build
- `macos-x64-release` - Intel release build

## Troubleshooting Checklist

- [ ] vcpkg triplet matches architecture?
- [ ] Runtime dependencies copied (DLLs on Windows)?
- [ ] Include paths set correctly?
- [ ] Link libraries in correct order?
- [ ] **ExternalProject dependencies explicitly declared?** (Critical for macOS)
- [ ] Platform-specific code wrapped in conditionals?
- [ ] CI cache keys include dependency version?

## Critical: Build Order Dependencies

When using ExternalProject_Add, **ALWAYS** add explicit dependencies:
```cmake
if(TARGET external-library-install)
    add_dependencies(your_target external-library-install)
endif()
```
Without this, macOS/Xcode builds will fail with missing library errors!

## Environment Variables

```bash
# Required for vcpkg
VCPKG_ROOT=C:\vcpkg  # or ~/vcpkg

# Optional overrides
VCPKG_TARGET_TRIPLET=arm64-osx
CMAKE_OSX_ARCHITECTURES=arm64
```

## File Locations

- Dependencies manifest: `vcpkg.json`
- vcpkg config: `vcpkg-configuration.json`  
- CMake presets: `CMakePresets.json`
- Main CMake: `CMakeLists.txt`
- Library CMake: `pers/CMakeLists.txt`
- Non-vcpkg libs: `third_party/`
- This guide: `docs/THIRD_PARTY_GUIDE.md`

## Contact & Support

For dependency issues:
1. Check `docs/THIRD_PARTY_GUIDE.md` for detailed instructions
2. Review CI logs for platform-specific errors
3. Verify local environment matches CI configuration