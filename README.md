# Pers Graphics Engine

A modern C++ graphics engine built with WebGPU.

## 📋 Project Structure

```
pers_graphics_engine/
├── README.md               # This file
├── LICENSE                 # MIT License
│
├── CMakeLists.txt          # Main build configuration
├── vcpkg.json              # Dependency manifest
├── vcpkg-configuration.json # Version pinning
│
├── pers/                   # Engine source code
│   ├── include/            # Public headers
│   └── src/                # Implementation
│
├── tests/                  # Test programs
│   ├── pers/               # Engine tests
│   └── third_party/        # Third-party integration tests
│
├── docs/                   # Documentation
│   └── renderer-core-design-v2/  # Architecture design
│
└── third_party/            # Dependency documentation
    └── README.md
```

## 🚀 Quick Start

### Prerequisites

1. **C++ Compiler**: Support for C++20
2. **CMake**: Version 3.20 or higher
3. **vcpkg** (Optional but recommended):
   ```bash
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.bat  # Windows
   ./bootstrap-vcpkg.sh   # Linux/macOS
   ```

### Building

```bash
# Clone the repository
git clone [repository-url]
cd pers_graphics_engine

# Build with vcpkg (recommended)
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build .

# Or build without vcpkg (limited features)
mkdir build && cd build
cmake ..
cmake --build .
```

## 📦 Dependencies

Managed via vcpkg manifest mode:
- **GLM** (0.9.9.8) - Mathematics library
- **GLFW** (3.3.8) - Window management
- **Dawn** (WebGPU) - Graphics API

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🛠️ Development Status

🚧 **Under Development** - APIs are subject to change.

See [docs/renderer-core-design-v2/](docs/renderer-core-design-v2/) for architecture documentation.