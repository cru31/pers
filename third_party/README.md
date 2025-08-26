# Third Party Dependencies

This project uses vcpkg manifest mode for dependency management.

## Dependencies

| Library | Version | Purpose | License |
|---------|---------|---------|---------|
| GLM | 0.9.9.8 | Mathematics library for graphics | MIT |
| GLFW | 3.3.8 | Window and input handling | zlib |
| Dawn | latest | WebGPU implementation | Apache-2.0 |

## Installation

Dependencies are automatically installed when you build the project with vcpkg toolchain.

### Prerequisites

1. Install vcpkg:
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Windows
.\bootstrap-vcpkg.bat

# Linux/macOS
./bootstrap-vcpkg.sh
```

2. Set environment variable (optional but recommended):
```bash
# Windows
set VCPKG_ROOT=C:\path\to\vcpkg

# Linux/macOS
export VCPKG_ROOT=/path/to/vcpkg
```

### Building with Dependencies

```bash
cd pers_graphics_engine
mkdir build && cd build

# Dependencies will be automatically installed to vcpkg_installed/
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

## Local Installation

Dependencies are installed to `vcpkg_installed/` directory in the project root:

```
pers_graphics_engine/
├── vcpkg.json                 # Manifest file (tracked in git)
├── vcpkg-configuration.json   # Version overrides (tracked in git)
└── vcpkg_installed/           # Local dependencies (git ignored)
    ├── x64-windows/
    │   ├── bin/
    │   ├── lib/
    │   └── include/
    └── vcpkg/
```

## Manual Dependency Management (Alternative)

If you prefer manual installation without vcpkg:

### GLM (Header-only)
- Download: https://github.com/g-truc/glm/releases/tag/0.9.9.8
- Extract to: `third_party/glm/`

### GLFW
- Download: https://www.glfw.org/download.html
- Build and install to system

### Dawn/WebGPU
- Repository: https://dawn.googlesource.com/dawn
- Follow build instructions: https://dawn.googlesource.com/dawn/+/HEAD/docs/building.md