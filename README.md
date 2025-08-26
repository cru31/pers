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

1. **Visual Studio 2022** or later with C++ development workload
2. **CMake**: Version 3.20 or higher
3. **vcpkg** (Required for dependency management):
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat
   ```
4. **Rust** (Required for WebGPU support):
   ```powershell
   # Install from https://rustup.rs
   # Or use winget:
   winget install Rust.Rustup
   ```

### Building

```bash
# Clone the repository
git clone [repository-url]
cd pers_graphics_engine

# Configure with CMake (vcpkg will auto-install dependencies)
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build the project
cmake --build build --config Debug

# Run tests
ctest --test-dir build -C Debug
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

### Using Pre-built WebGPU
To avoid Rust dependency, you can use pre-built wgpu-native:
1. Download from [wgpu-native releases](https://github.com/gfx-rs/wgpu-native/releases/tag/v0.19.4.1)
2. Extract to `third_party/wgpu-native-bin/`
3. CMake will automatically detect and use pre-built binaries

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