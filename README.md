# Pers Graphics Engine

A modern C++ graphics engine built with WebGPU.

## 📋 Project Structure

```
pers_graphics_engine/
├── README.md                # This file
├── LICENSE                  # MIT License
├── BUILD_ENV.md            # Build environment details
│
├── CMakeLists.txt          # Main build configuration
├── vcpkg.json              # Dependency manifest
├── vcpkg-configuration.json # Version pinning
├── rust-toolchain.toml     # Rust version lock for wgpu-native
│
├── pers/                   # Engine source code
│   ├── include/            # Public headers
│   │   └── pers/
│   │       └── graphics/   # Graphics subsystem
│   └── src/                # Implementation
│
├── tests/                  # Test programs
│   ├── pers/               # Engine tests
│   └── third_party/        # Third-party integration tests
│
├── docs/                   # Documentation
│   └── renderer-core-design-v2/  # Architecture design
│
└── third_party/            # External dependencies
    ├── wgpu-native-bin/    # Pre-built WebGPU (optional)
    └── README.md
```

## 🚀 Quick Start

### Prerequisites

1. **C++ Compiler**: 
   - Windows: Visual Studio 2022 or later
   - Linux: GCC 11+ or Clang 14+
   - macOS: Xcode 14+ or Apple Clang
2. **CMake**: Version 3.20 or higher
3. **vcpkg** (Required for dependency management):
   ```bash
   # Windows
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat
   
   # Linux/macOS
   git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg
   ~/vcpkg/bootstrap-vcpkg.sh
   ```
4. **Rust** (Optional - for building wgpu-native from source):
   ```bash
   # Install from https://rustup.rs
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   ```
   Note: If Rust is not installed, pre-built WebGPU binaries will be downloaded automatically.

### Building

#### Basic Build (Core Library Only)
```bash
# Clone the repository
git clone https://github.com/cru31/pers.git
cd pers

# Configure with CMake (vcpkg will auto-install dependencies)
# Windows
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# Linux/macOS
cmake -B build -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build the project
cmake --build build --config Debug   # For development/debugging
# or
cmake --build build --config Release # For performance testing
```

#### Build with Tests (includes GLFW)
```bash
# Install GLFW via vcpkg feature
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake \
      -DVCPKG_MANIFEST_FEATURES="tests" -DBUILD_TESTS=ON

# Build and run tests
cmake --build build --config Debug
ctest --test-dir build -C Debug
```

#### Build with Samples
```bash
# Install GLFW for samples
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake \
      -DVCPKG_MANIFEST_FEATURES="samples" -DBUILD_SAMPLES=ON

cmake --build build --config Debug
```

### Quick Test

After building, test the libraries:
```bash
# Test static library
./build/bin/Debug/test_pers_static.exe

# Test shared library
./build/bin/Debug/test_pers_shared.exe

# Test WebGPU integration
./build/bin/Debug/test_pers_webgpu.exe
```

## 📦 Dependencies

### Core Dependencies (Required)
Managed automatically via vcpkg manifest mode:
- **GLM** (1.0.1#1) - Graphics mathematics library
- **wgpu-native** (v0.19.4.1) - WebGPU implementation
  - Can use pre-built binaries or build from source
  - Requires Rust 1.79.0 for source builds

### Optional Dependencies
- **GLFW** - Window management (for samples/tests only)
  - Automatically installed when using `-DVCPKG_MANIFEST_FEATURES="tests"` or `"samples"`
  - Not needed for core library

### WebGPU Build Options
1. **Automatic (Recommended)**: CMake automatically handles WebGPU:
   - If Rust is available: Builds from source (~1-2 minutes)
   - If Rust is not available: Downloads pre-built binaries automatically
   
2. **Manual Pre-built**: To force using specific pre-built binaries:
   - Download from [wgpu-native releases](https://github.com/gfx-rs/wgpu-native/releases/tag/v0.19.4.1)
   - Extract to `third_party/wgpu-native-bin/`

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🛠️ Development Status

🚧 **Under Development** - APIs are subject to change.

### Documentation
- [Architecture Design](docs/renderer-core-design-v2/) - Core renderer design
- [Build Environment](BUILD_ENV.md) - Detailed build requirements and versions

### Troubleshooting

For common build issues, see [BUILD_ENV.md](BUILD_ENV.md#troubleshooting).

For other issues, please report at the project's issue tracker.