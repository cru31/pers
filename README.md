# Pers Graphics Engine

A modern C++ graphics engine built with WebGPU.

## 📋 Project Structure

```
pers_graphics_engine/
├── README.md                # This file
├── LICENSE                  # MIT License
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
│   ├── getting-started/    # Setup and build guides
│   ├── design/             # Architecture documentation
│   │   └── renderer-core/  # Core renderer design
│   ├── api/                # API documentation
│   ├── guides/             # User guides
│   └── troubleshooting/    # Problem solving
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
   - Required version: 1.82.0 or later
   ```bash
   # Install from https://rustup.rs
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   
   # The project uses rust-toolchain.toml to pin Rust version to 1.82.0
   # This will be automatically installed when building from source
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
- **GLM** (1.0.1#3) - Graphics mathematics library
- **wgpu-native** (v25.0.2.1) - WebGPU implementation
  - Can use pre-built binaries or build from source
  - Requires Rust 1.82.0+ for source builds

### Optional Dependencies
- **GLFW** - Window management (for samples/tests only)
  - Automatically installed when using `-DVCPKG_MANIFEST_FEATURES="tests"` or `"samples"`
  - Not needed for core library

### WebGPU Build Options

The project supports two WebGPU build methods, organized in separate directories:

1. **Pre-built Binaries (Default)**: 
   - Downloads pre-built wgpu-native binaries automatically
   - Stored in `third_party/wgpu-native-runtime/prebuilt/`
   - Build output: `build/wgpu-prebuilt/`
   - Use CMake preset: `cmake --preset=windows-debug-download`

2. **Build from Source**:
   - Requires Rust 1.82.0+ (automatically installed via rust-toolchain.toml)
   - Compiles wgpu-native from source (~5-10 minutes first build)
   - Stored in `third_party/wgpu-native-runtime/custom-build/`
   - Build output: `build/wgpu-compile/`
   - Use CMake preset: `cmake --preset=windows-debug-compile`

Available CMake presets:
- `windows-debug-download` - Debug build with pre-built WebGPU
- `windows-release-download` - Release build with pre-built WebGPU
- `windows-debug-compile` - Debug build compiling WebGPU from source
- `windows-release-compile` - Release build compiling WebGPU from source

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🛠️ Development Status

🚧 **Under Development** - APIs are subject to change.

### Documentation
- [Getting Started](docs/getting-started/) - Setup and build guides
- [Architecture Design](docs/design/renderer-core/) - Core renderer design
- [API Documentation](docs/api/) - API reference

### Troubleshooting

For common build issues, see [Build Environment Guide](docs/getting-started/BUILD_ENV.md#troubleshooting).

For other issues, please report at the project's issue tracker.