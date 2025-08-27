# Building Pers Graphics Engine on macOS

This guide covers building the Pers Graphics Engine on macOS, including handling of dynamic library dependencies.

## Prerequisites

### Required Tools
- CMake 3.20 or higher
- Xcode Command Line Tools
- vcpkg (for dependency management)
- Git

### Optional Tools
- Rust toolchain (for building wgpu-native from source)
  - Install via: `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`

## Architecture Considerations

The build system automatically detects your macOS architecture (ARM64 for Apple Silicon, x86_64 for Intel). You can explicitly specify the architecture:

```bash
# For Apple Silicon (M1/M2/M3)
cmake -B build -DCMAKE_OSX_ARCHITECTURES=arm64

# For Intel Macs
cmake -B build -DCMAKE_OSX_ARCHITECTURES=x86_64
```

## Building with vcpkg

### 1. Install vcpkg
```bash
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
```

### 2. Set Environment Variables
```bash
export VCPKG_ROOT=/path/to/vcpkg
export PATH=$VCPKG_ROOT:$PATH
```

### 3. Install Dependencies
The project uses vcpkg manifest mode. Dependencies will be installed automatically during CMake configuration:

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
```

## WebGPU Backend Options

The project supports two methods for obtaining wgpu-native:

### Option 1: Pre-built Binaries (Default)
Downloads pre-built wgpu-native binaries from GitHub releases. This is the default if Rust is not available.

```bash
cmake -B build -DFORCE_WGPU_DOWNLOAD=ON
```

### Option 2: Build from Source
Builds wgpu-native from source using Rust. Requires Rust toolchain.

```bash
cmake -B build -DFORCE_WGPU_COMPILE=ON
```

## Dynamic Library Management

### macOS Dynamic Library Issues

On macOS, dynamically linked libraries (dylibs) can have path issues due to how macOS handles library loading. The project includes automatic fixes for these issues.

#### The Problem
When building wgpu-native from source with Rust's cargo, the resulting dylib may contain hardcoded absolute paths that won't work on other machines or in CI environments.

#### The Solution
The build system automatically:
1. Copies wgpu-native dylib to a consistent location
2. Updates the dylib's install name to use `@rpath`
3. Fixes references in all executables using `install_name_tool`

### Implementation Details

The project includes `cmake/MacOSFixDylib.cmake` which provides:
- `fix_macos_dylib_references(target_name)`: Fixes dylib references for a single target
- `fix_macos_dylib_for_targets(...)`: Fixes dylib references for multiple targets

These functions:
1. Detect which wgpu-native build method was used (custom-build or prebuilt)
2. Set appropriate RPATH values for the target
3. Fix hardcoded dylib paths if needed (only for custom builds)

### RPATH Configuration

The build sets the following RPATH values:
- `@loader_path`: Directory containing the executable
- `@loader_path/../lib`: Library directory relative to executable
- Absolute path to wgpu-native runtime directory (fallback)

## Build Commands

### Standard Build
```bash
# Configure
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build

# Run tests
ctest --test-dir build
```

### Debug Build with Verbose Output
```bash
cmake -B build-debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_VERBOSE_MAKEFILE=ON

cmake --build build-debug --verbose
```

## Troubleshooting

### Library Not Found Errors

If you encounter "Library not loaded" errors:

1. **Check dylib paths**:
   ```bash
   otool -L path/to/executable
   ```

2. **Verify RPATH settings**:
   ```bash
   otool -l path/to/executable | grep -A 2 LC_RPATH
   ```

3. **Check dylib install name**:
   ```bash
   otool -D path/to/library.dylib
   ```

### Architecture Mismatch

If you get architecture mismatch errors:

1. **Verify vcpkg triplet**:
   ```bash
   # For Apple Silicon
   export VCPKG_DEFAULT_TRIPLET=arm64-osx
   
   # For Intel
   export VCPKG_DEFAULT_TRIPLET=x64-osx
   ```

2. **Check binary architecture**:
   ```bash
   file path/to/binary
   lipo -info path/to/binary
   ```

### Fixing Dylib Paths Manually

If automatic fixes don't work, you can manually fix dylib paths:

```bash
# Fix install name of the dylib itself
install_name_tool -id "@rpath/libwgpu_native.dylib" path/to/libwgpu_native.dylib

# Fix references in executable
install_name_tool -change /old/path/to/libwgpu_native.dylib \
  @rpath/libwgpu_native.dylib \
  path/to/executable

# Add RPATH to executable
install_name_tool -add_rpath @loader_path path/to/executable
install_name_tool -add_rpath /absolute/path/to/lib path/to/executable
```

## CI/CD Considerations

### GitHub Actions

The project's CI workflow handles macOS builds with:
1. Architecture detection based on runner type
2. Proper vcpkg triplet selection
3. Automatic dylib path fixing

Key CI environment variables:
```yaml
VCPKG_DEFAULT_TRIPLET: ${{ matrix.triplet }}
VCPKG_TARGET_TRIPLET: ${{ matrix.triplet }}
```

### Caching

To speed up CI builds, consider caching:
- vcpkg binary cache: `~/.cache/vcpkg`
- wgpu-native runtime: `third_party/wgpu-native-runtime/`

## Testing Dynamic Linking

After building, verify dynamic linking is working:

```bash
# Run test executables
./build/bin/test_pers_static
./build/bin/test_pers_shared
./build/bin/test_pers_webgpu

# Check for missing dependencies
for exe in build/bin/test_*; do
  echo "Checking $exe..."
  otool -L "$exe" | grep -E "not found|missing"
done
```

## Directory Structure

After successful build:
```
build/
├── bin/                    # Executables
│   ├── test_pers_static
│   ├── test_pers_shared
│   └── test_pers_webgpu
├── lib/                    # Libraries
│   ├── libpers_static.a
│   └── libpers.dylib
└── pers/                   # Build artifacts
    └── wgpu-native-src/    # (if building from source)

third_party/
└── wgpu-native-runtime/
    ├── custom-build/       # Built from source
    │   ├── include/
    │   └── lib/
    │       └── libwgpu_native.dylib
    └── prebuilt/           # Downloaded binaries
        ├── include/
        └── lib/
            └── libwgpu_native.dylib
```

## Additional Resources

- [Apple Developer: Dynamic Library Programming Topics](https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/DynamicLibraries/000-Introduction/Introduction.html)
- [CMake RPATH handling](https://cmake.org/cmake/help/latest/prop_tgt/INSTALL_RPATH.html)
- [wgpu-native GitHub](https://github.com/gfx-rs/wgpu-native)