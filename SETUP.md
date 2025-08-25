# Pers Graphics Engine Setup Guide

## Prerequisites

### Required Software
- **CMake** 3.20 or higher
- **Visual Studio 2022** with C++ development workload
- **Rust** (for building wgpu-native)
  - Install from: https://rustup.rs/
- **LLVM** (for RC compiler)
  - Install from: https://github.com/llvm/llvm-project/releases
  - Add to PATH: `C:\Program Files\LLVM\bin`
- **Git**

### Optional Software
- **Ninja build system** (included with Visual Studio)

## Initial Setup

### 1. Clone the Repository
```bash
git clone https://github.com/cru31/pers.git
cd pers
```

### 2. Setup Third-Party Dependencies

#### Build wgpu-native from source:
```bash
# Create third_party directory
mkdir third_party
cd third_party

# Clone wgpu-native
git clone --recursive https://github.com/gfx-rs/wgpu-native.git
cd wgpu-native

# Build wgpu-native (this will take some time)
cargo build --release

# The built library will be at:
# Windows: target/release/wgpu_native.dll and wgpu_native.dll.lib
# macOS: target/release/libwgpu_native.dylib
# Linux: target/release/libwgpu_native.so
```

#### Clone GLFW:
```bash
cd ../  # Back to third_party
git clone https://github.com/glfw/glfw.git
```

## Building Test Programs

### WebGPU Tests

The WebGPU test programs demonstrate basic WebGPU functionality.

#### 1. Initialization Test
Tests basic WebGPU instance and device creation:

```bash
cd tests/third_party/webgpu/initialization
mkdir build
cd build
cmake ..
cmake --build . --config Debug
cmake --build . --config Release

# Run the test
./Debug/webgpu_init.exe    # or ./Release/webgpu_init.exe
```

#### 2. Triangle Test
Renders a triangle using WebGPU:

```bash
cd tests/third_party/webgpu/triangle
mkdir build
cd build
cmake ..
cmake --build . --config Debug
cmake --build . --config Release

# Run the test (ESC to exit)
./Debug/webgpu_triangle.exe    # or ./Release/webgpu_triangle.exe
```

## Project Structure

```
pers_graphics_engine/
├── docs/                       # Design documentation
│   └── renderer-core-design/   # Renderer architecture docs
├── tests/                      # Test programs
│   └── third_party/           # Third-party library tests
│       └── webgpu/            # WebGPU test programs
│           ├── initialization/ # Basic init test
│           └── triangle/      # Triangle rendering test
├── third_party/               # External dependencies (not in repo)
│   ├── wgpu-native/          # WebGPU implementation
│   └── glfw/                 # Window management
└── SETUP.md                  # This file
```

## Troubleshooting

### Build Errors

1. **"RC compiler not found"**
   - Install LLVM and ensure `llvm-rc.exe` is in PATH
   - Or use Visual Studio generator instead of Ninja

2. **"wgpu_native.dll not found"**
   - Ensure wgpu-native is built in Release mode
   - Check that the DLL is copied to the executable directory

3. **"Failed to create WebGPU instance"**
   - Update your graphics drivers
   - Ensure your GPU supports DirectX 12 (Windows) or Metal (macOS)

### Runtime Issues

1. **Window doesn't appear**
   - Check that GLFW is properly linked
   - Verify that the shader file is in the same directory as the executable

2. **Triangle not visible**
   - Press ESC to exit and check console output for errors
   - Verify that `shader.wgsl` is present in the executable directory

## Development Notes

- All test programs use **camelCase** naming convention
- Build both Debug and Release configurations for testing
- The engine core will be platform-independent
- WebGPU is used as the primary graphics API

## Contributing

See `docs/renderer-core-design/` for architecture documentation and design principles.