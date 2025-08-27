# Build Environment Requirements

## Pers Graphics Engine v0.1.0

### Required Tools and Versions

#### Core Build Tools
- **CMake**: 3.20 or higher
- **Visual Studio**: 2022 (17.0+) with C++ development workload
- **Windows SDK**: 10.0.22000.0 or higher
- **vcpkg**: Latest version with manifest mode support
  - Baseline: `6ecbbbdf31cba47aafa7cf6189b1e73e10ac61f8`

#### Language Requirements
- **C++ Standard**: C++20
- **Rust**: 1.82.0 (specified in rust-toolchain.toml)
  - Target: `x86_64-pc-windows-msvc`
  - Required for building wgpu-native from source

### Required Dependencies

#### Package Dependencies (via vcpkg)
- **GLM**: 1.0.1#1
  - Graphics math library
  - Required for all builds

#### WebGPU Implementation
- **wgpu-native**: v25.0.2.1
  - Can use pre-built binaries OR build from source
  - Source build requires Rust toolchain
  - Pre-built automatically downloaded to: `build/pers/wgpu-native-prebuilt/`

### Version Lock Details

#### wgpu-native v25.0.2.1
- **Repository**: https://github.com/gfx-rs/wgpu-native
- **Tag**: v25.0.2.1
- **Release Date**: August 2025
- **API Changes**: Uses Future-based async API
- **Directory Structure**: 
  - Headers in: `include/webgpu/`
  - Libraries in: `lib/`

#### Rust Toolchain (rust-toolchain.toml)
```toml
[toolchain]
channel = "1.82.0"
components = ["rustfmt", "clippy"]
targets = ["x86_64-pc-windows-msvc"]
profile = "minimal"
```

### Build Configurations Tested

#### Windows 11 Build
- **Date**: 2025-08-27
- **OS**: Windows 11 23H2 (Build 22631)
- **Compiler**: MSVC 19.44.35207.0
- **CPU Architecture**: x64
- **Build Type**: Debug/Release
- **WebGPU Version**: wgpu-native v25.0.2.1

### Directory Structure

```
pers_graphics_engine/
├── rust-toolchain.toml      # Rust version lock
├── vcpkg.json               # vcpkg manifest
├── vcpkg-configuration.json # vcpkg version pinning
├── CMakeLists.txt           # Root CMake
├── pers/                    # Engine library
│   ├── CMakeLists.txt
│   ├── include/pers/        # Public headers
│   └── src/                 # Implementation
├── tests/                   # Test programs
├── third_party/            # External dependencies
│   └── glm/                # GLM math library
├── build/                   # Build output (generated)
│   ├── wgpu-prebuilt/       # Pre-built WebGPU binaries
│   └── wgpu-compile/        # Source-built WebGPU
└── third_party/
    └── wgpu-native-runtime/ # WebGPU runtime storage
        ├── prebuilt/        # Downloaded binaries
        └── custom-build/    # Built from source
```

### Build Instructions

#### First-Time Setup
1. **Install Visual Studio 2022** with C++ workload
2. **Install vcpkg**:
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat
   ```

3. **Install Rust** (for WebGPU):
   ```powershell
   # Download from https://rustup.rs
   # Or use winget:
   winget install Rust.Rustup
   ```

#### Build Commands

##### Using CMake Presets (Recommended)
```bash
# Pre-built WebGPU (faster)
cmake --preset=windows-debug-download
cmake --build build/wgpu-prebuilt/debug --config Debug

# Build from source (requires Rust)
cmake --preset=windows-debug-compile
cmake --build build/wgpu-compile/debug --config Debug
```

##### Manual Configuration
```bash
# Configure with pre-built WebGPU
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DFORCE_WGPU_DOWNLOAD=ON

# Configure with source build
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DFORCE_WGPU_COMPILE=ON

# Build and test
cmake --build build --config Debug
ctest --test-dir build -C Debug
```

### WebGPU Build Options

#### Using Pre-built wgpu-native (Default)
The build system automatically downloads pre-built wgpu-native binaries if Rust is not installed.
- **Automatic download**: CMake fetches platform-specific binaries
- **Version**: v25.0.2.1
- **Runtime Location**: `third_party/wgpu-native-runtime/prebuilt/`
- **Build Output**: `build/wgpu-prebuilt/`
- **CMake Option**: `-DFORCE_WGPU_DOWNLOAD=ON`

#### Building from Source
If Rust is available, wgpu-native can be built from source.
- **Requires**: Rust 1.82.0+
- **Runtime Location**: `third_party/wgpu-native-runtime/custom-build/`
- **Build Output**: `build/wgpu-compile/`
- **CMake Option**: `-DFORCE_WGPU_COMPILE=ON`
- **Platform naming**:
  - Windows: `windows-x86_64-msvc`
  - macOS: `macos-universal`
  - Linux: `linux-x86_64`

### Troubleshooting

#### Rust Not Found
- **Error**: "Rust compiler not found"
- **Solution**: Install from https://rustup.rs

#### vcpkg Baseline Mismatch
- **Error**: "builtin-baseline mismatch"
- **Solution**: Update vcpkg.json baseline to match vcpkg-configuration.json

#### WebGPU Link Errors
- **Error**: LNK2019 unresolved external symbol
- **Solution**: 
  1. Check if wgpu-native built successfully
  2. Pre-built binaries are automatically downloaded if Rust is not installed
  3. Check `build/pers/wgpu-native-prebuilt/` for downloaded files

### CI/CD Configuration

For automated builds, use these environment variables:
```yaml
env:
  VCPKG_ROOT: C:/vcpkg
  CMAKE_TOOLCHAIN_FILE: C:/vcpkg/scripts/buildsystems/vcpkg.cmake
  RUST_VERSION: 1.82.0
  WGPU_NATIVE_VERSION: v25.0.2.1
```

### Version Upgrade Guidelines

When upgrading dependencies:
1. Update version in CMakeLists.txt
2. Update rust-toolchain.toml if Rust version changes
3. Update vcpkg.json for vcpkg packages
4. Update this BUILD_ENV.md file
5. Test on clean environment
6. Update CI/CD configurations

### Contact

For build issues, please report at: https://github.com/user/pers_graphics_engine/issues