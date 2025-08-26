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
- **Rust**: 1.79.0 (specified in rust-toolchain.toml)
  - Target: `x86_64-pc-windows-msvc`
  - Required for building wgpu-native from source

### Required Dependencies

#### Package Dependencies (via vcpkg)
- **GLM**: 1.0.1#1
  - Graphics math library
  - Required for all builds

#### WebGPU Implementation
- **wgpu-native**: v0.19.4.1
  - Can use pre-built binaries OR build from source
  - Source build requires Rust toolchain
  - Pre-built location: `third_party/wgpu-native-bin/`

### Version Lock Details

#### wgpu-native v0.19.4.1
- **Repository**: https://github.com/gfx-rs/wgpu-native
- **Tag**: v0.19.4.1
- **Dependencies**:
  - wgpu: 0.19.3
  - wgpu-core: 0.19.3
  - wgpu-hal: 0.19.3
  - naga: 0.19.2

#### Rust Toolchain (rust-toolchain.toml)
```toml
[toolchain]
channel = "1.79.0"
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
│   └── wgpu-native-bin/    # Pre-built WebGPU (optional)
└── build/                   # Build output (generated)
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
```bash
# Configure (first time)
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build --config Debug

# Run tests
ctest --test-dir build -C Debug
```

### Using Pre-built wgpu-native

To avoid Rust dependency, download pre-built wgpu-native:
1. Download from: https://github.com/gfx-rs/wgpu-native/releases/tag/v0.19.4.1
2. Extract to: `third_party/wgpu-native-bin/`
3. Files needed:
   - `wgpu_native.dll`
   - `wgpu_native.dll.lib`
   - `wgpu.h` (header file)

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
  2. Or use pre-built binaries in `third_party/wgpu-native-bin/`

### CI/CD Configuration

For automated builds, use these environment variables:
```yaml
env:
  VCPKG_ROOT: C:/vcpkg
  CMAKE_TOOLCHAIN_FILE: C:/vcpkg/scripts/buildsystems/vcpkg.cmake
  RUST_VERSION: 1.79.0
  WGPU_NATIVE_VERSION: v0.19.4.1
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