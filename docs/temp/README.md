# Pers Graphics Engine - Architecture Diagrams

## Overview
This directory contains comprehensive UML diagrams documenting the current architecture of the Pers Graphics Engine, including all recent updates and implementations.

## Diagrams Included

### 1. Class Diagram (`pers_class_diagram.puml`)
Complete class hierarchy showing:
- **Core Interfaces**: IInstance, IPhysicalDevice, ILogicalDevice, IResourceFactory, etc.
- **WebGPU Implementations**: All WebGPU backend classes
- **Framebuffer Architecture**: SurfaceFramebuffer and OffscreenFramebuffer
- **Helper Classes**: Converters, Builders, Configurations
- **Data Structures**: All descriptors and state objects
- **Enumerations**: Complete texture formats (including BC1-BC7), buffer usage, etc.
- **Relationships**: Creation, ownership, and usage relationships

Key Updates Reflected:
- DepthStencilState now includes `format` field (no more hardcoding)
- All WebGPU-supported texture formats including new ones (RGB9E5Ufloat, RGB10A2Unorm, etc.)
- BC compressed formats (BC1-BC7)
- IndexFormat with Undefined value

### 2. Sequence Diagrams (`pers_sequence_diagrams.puml`)
Five comprehensive sequence diagrams covering all major workflows:

#### 2.1 Initialization Sequence
Complete graphics system initialization flow:
- Instance creation
- Surface creation
- Physical device selection and capability check
- Logical device creation
- SwapChain setup
- Framebuffer configuration with depth attachment

#### 2.2 Resource Creation Sequence
Detailed resource creation flows:
- Buffer creation with size validation
- Texture creation with format conversion
- Shader module compilation
- Render pipeline creation with depth-stencil configuration

#### 2.3 Frame Rendering Sequence
Complete frame rendering pipeline:
- Image acquisition
- Command encoder creation
- Render pass configuration with depth attachment
- Draw command recording
- Command submission
- Frame presentation

#### 2.4 Format Conversion Sequence
Texture format handling:
- Standard format conversion (Depth24Plus, Depth32Float)
- Unsupported format fallback (R16Unorm → R16Float)
- BC compressed format support
- Format mismatch detection

#### 2.5 Error Handling Sequence
Validation and error scenarios:
- Buffer size validation (size > 0 requirement)
- Device lost handling
- Pipeline/RenderPass format mismatch detection
- WGPUStringView proper construction

## Key Architectural Features

### 1. No Hardcoding
- All formats are user-specified
- DepthStencilState.format field allows custom depth formats
- No assumptions about texture formats

### 2. Complete Format Support
- All WebGPU standard formats
- BC1-BC7 compressed formats for desktop
- Fallback mechanisms for unsupported formats
- Proper format conversion in both directions

### 3. Clean Separation of Concerns
- Interface/Implementation separation
- Backend-specific code isolated in WebGPU classes
- Platform-independent framebuffer architecture

### 4. Resource Management
- Proper RAII with shared_ptr/weak_ptr
- Factory pattern for resource creation
- Builder pattern for complex descriptors

### 5. Error Handling
- Comprehensive validation
- Detailed logging
- Graceful fallback for unsupported features

## Recent Changes Reflected

1. **Texture Format Expansion**
   - Added R16Unorm, R16Snorm, RG16Unorm, RG16Snorm
   - Added RGB9E5Ufloat, RGB10A2Unorm, RG11B10Ufloat
   - Added RGBA16Unorm, RGBA16Snorm
   - Added all BC compressed formats (BC1-BC7)

2. **Hardcoding Removal**
   - DepthStencilState now has format field
   - No more hardcoded Depth24PlusStencil8
   - No more hardcoded BGRA8Unorm assumptions

3. **Bug Fixes**
   - IndexFormat enum duplication resolved
   - WGPUStringView assignment fixed
   - Pipeline/RenderPass format mismatch handled

4. **Validation Improvements**
   - Buffer size > 0 validation
   - Format compatibility checks
   - Device availability verification

## How to View

These PlantUML diagrams can be viewed using:
1. **VSCode**: Install PlantUML extension
2. **Online**: http://www.plantuml.com/plantuml/
3. **Command Line**: `plantuml *.puml` to generate PNG/SVG

## Notes

- All diagrams reflect the current state as of 2025-01-09
- WebGPU backend is fully functional with the shown architecture
- Future backends (Vulkan, Metal, D3D12) will follow the same interface structure