# Documentation Standards Guide

## Overview
This document defines the standards and guidelines for writing and maintaining documentation for the Pers Graphics Engine library. All contributors must follow these standards to ensure consistency and quality across the documentation.

## When to Update Documentation

### Mandatory Documentation Updates
Documentation MUST be updated when:
- **New API Added**: Any new public interface, class, or function
- **API Changed**: Signature changes, parameter modifications, behavior changes
- **Breaking Changes**: Any change that breaks backward compatibility
- **Feature Addition**: New capabilities or features added to the engine
- **Bug Fix**: If the fix changes expected behavior or adds new constraints
- **Platform Support**: Adding or removing platform support
- **Dependencies**: Changes in external dependencies or requirements

### Documentation Update Checklist
- [ ] API reference updated
- [ ] Usage examples updated or added
- [ ] Migration guide updated (if breaking change)
- [ ] Platform-specific notes added (if applicable)
- [ ] Performance implications documented
- [ ] Limitations and constraints documented
- [ ] Related documentation cross-referenced

## Documentation Structure Standards

### File Naming Conventions
- Use lowercase with hyphens: `physical-device.md` (NOT `PhysicalDevice.md`)
- Be descriptive but concise: `device-creation.md` (NOT `how-to-create-devices.md`)
- Use consistent terminology throughout

### Standard Document Template

```markdown
# [Component/Feature Name]

> **Status**: [Stable | Beta | Experimental | Deprecated]
> **Since**: v[X.Y.Z]
> **Backend**: [All | WebGPU | Vulkan | Metal | D3D12]

## Overview
[1-2 paragraphs describing what this component does and why it exists]

## Table of Contents
- [Key Concepts](#key-concepts)
- [API Reference](#api-reference)
- [Usage](#usage)
- [Platform Notes](#platform-notes)
- [Limitations](#limitations)
- [Performance](#performance)
- [Troubleshooting](#troubleshooting)
- [See Also](#see-also)

## Key Concepts
[Explain fundamental concepts users need to understand]

## API Reference

### Classes/Interfaces
#### `ClassName`
[Brief description]

**Header**: `pers/path/to/header.h`

### Methods

#### `methodName`
```cpp
ReturnType methodName(ParamType param) const;
```

**Parameters:**
- `param`: [Description, constraints, valid ranges]

**Returns:**
- [Description of return value and possible states]

**Exceptions:**
- `ExceptionType`: [When thrown]

**Thread Safety:**
- [Thread safety guarantees]

**Example:**
```cpp
// Example code with comments
auto result = object->methodName(value);
```

### Enumerations

#### `EnumName`
| Value | Description | Notes |
|-------|-------------|-------|
| `Value1` | Description | Additional notes |
| `Value2` | Description | Platform-specific |

## Usage

### Basic Usage
[Simple, common use case]

### Advanced Usage
[Complex scenarios, optimization techniques]

## Platform Notes

### Windows
[Windows-specific information]

### Linux
[Linux-specific information]

### macOS
[macOS-specific information]

## Limitations

### Current Limitations
- [List known limitations]
- [Workarounds if available]

### Not Supported
- [Features explicitly not supported]

## Performance

### Performance Characteristics
- Time Complexity: O(n)
- Space Complexity: O(1)
- Allocation Behavior: [Describe]

### Optimization Tips
- [Performance best practices]

## Troubleshooting

### Common Issues
| Issue | Cause | Solution |
|-------|-------|----------|
| Error message | Likely cause | How to fix |

## Version History
| Version | Changes | Migration Notes |
|---------|---------|-----------------|
| v1.0.0 | Initial release | - |
| v1.1.0 | Added feature X | No breaking changes |

## See Also
- [Related Component](../path/to/doc.md)
- [Usage Guide](../guides/guide.md)
```

## Code Example Standards

### Example Requirements
All code examples must:
1. **Compile**: Be syntactically correct and compilable
2. **Be Complete**: Include necessary headers and context
3. **Have Comments**: Explain each significant step
4. **Handle Errors**: Show proper error handling
5. **Follow Style**: Match project coding standards

### Example Template
```cpp
// Include required headers
#include "pers/graphics/IInstance.h"
#include "pers/graphics/IPhysicalDevice.h"

// Step 1: Explain what this step does
auto instance = factory->createInstance(desc);
if (!instance) {
    // Handle error appropriately
    return false;
}

// Step 2: Next operation with explanation
PhysicalDeviceOptions options;
options.powerPreference = PowerPreference::HighPerformance;
options.compatibleSurface = surface;  // Important: surface must be valid

// Step 3: Show result handling
auto device = instance->requestPhysicalDevice(options);
if (!device) {
    // Explain common failure reasons
    // - No compatible adapter
    // - Surface not supported
    return false;
}
```

## API Documentation Standards

### Function Documentation
```cpp
/**
 * @brief Brief one-line description
 * 
 * Detailed description explaining the function's purpose,
 * behavior, and any important notes.
 * 
 * @param paramName Description of parameter, valid ranges, constraints
 * @param[out] outParam Description of output parameter
 * @param[in,out] inOutParam Description of input/output parameter
 * 
 * @return Description of return value, nullptr conditions
 * 
 * @throws ExceptionType When this exception is thrown
 * 
 * @note Important information about usage
 * @warning Warnings about potential issues
 * @deprecated Since v2.0.0, use newFunction() instead
 * 
 * @see RelatedFunction
 * @since v1.0.0
 * 
 * @code
 * // Example usage
 * auto result = function(param);
 * @endcode
 */
```

### Class Documentation
```cpp
/**
 * @brief Brief one-line description
 * 
 * Detailed description of the class purpose and responsibilities.
 * 
 * This class is responsible for:
 * - First responsibility
 * - Second responsibility
 * 
 * @note Thread-safety information
 * @warning Important usage warnings
 * 
 * @see RelatedClass
 * @since v1.0.0
 */
```

## Markdown Standards

### Headers
- Use ATX-style headers (`#` not underlines)
- Leave blank line before and after headers
- Don't skip header levels (# → ## → ###)

### Lists
- Use `-` for unordered lists
- Use `1.` for ordered lists
- Indent nested lists with 2 spaces

### Code Blocks
- Always specify language for syntax highlighting
- Use \`\`\`cpp for C++ code
- Use \`\`\`bash for shell commands
- Use \`\`\`json for JSON data

### Tables
- Use pipes and hyphens for tables
- Align columns for readability in source
- Include header row

### Links
- Use relative links for internal documentation
- Use descriptive link text (not "click here")
- Check links regularly for validity

## Documentation Review Checklist

### Technical Accuracy
- [ ] Code examples compile and run
- [ ] API signatures are correct
- [ ] Platform-specific information is accurate
- [ ] Performance characteristics are verified

### Completeness
- [ ] All public APIs are documented
- [ ] All parameters are described
- [ ] Return values are explained
- [ ] Error conditions are listed
- [ ] Examples cover common use cases

### Clarity
- [ ] Language is clear and concise
- [ ] Technical terms are defined
- [ ] Complex concepts have examples
- [ ] Formatting is consistent

### Maintenance
- [ ] Version information is current
- [ ] Deprecated features are marked
- [ ] Migration paths are provided
- [ ] Cross-references are valid

## Version Control

### Commit Messages for Documentation
```
docs: Add documentation for WebGPU device creation

- Add comprehensive device creation guide
- Include platform-specific notes
- Add troubleshooting section
```

### Documentation Versioning
- Tag documentation with release versions
- Maintain documentation for supported versions
- Archive documentation for EOL versions

## Tools and Validation

### Required Tools
- Markdown linter (markdownlint)
- Link checker (markdown-link-check)
- Spell checker (cspell)

### Validation Commands
```bash
# Lint markdown files
markdownlint docs/**/*.md

# Check links
markdown-link-check docs/**/*.md

# Spell check
cspell docs/**/*.md
```

## Special Documentation Types

### Migration Guides
Must include:
- Version compatibility matrix
- Breaking changes list
- Step-by-step migration process
- Code diff examples
- Common pitfalls

### Troubleshooting Guides
Must include:
- Problem symptoms
- Root causes
- Step-by-step solutions
- Preventive measures
- When to seek help

### Performance Guides
Must include:
- Benchmarking methodology
- Performance metrics
- Optimization techniques
- Trade-offs
- Profiling instructions

## Documentation Maintenance

### Regular Reviews
- Quarterly review of all documentation
- Update examples to use latest APIs
- Verify platform-specific information
- Check and fix broken links

### User Feedback Integration
- Track documentation issues
- Prioritize based on user impact
- Regular updates based on FAQ
- Include real-world examples

## Quick Reference

### Documentation Types Priority
1. **Critical**: API changes, breaking changes
2. **High**: New features, bug fixes affecting behavior
3. **Medium**: Performance improvements, minor features
4. **Low**: Internal changes, refactoring

### Time Requirements
- New API: Document before merge
- Breaking change: Document + migration guide before merge
- Bug fix: Document if behavior changes
- Feature: Complete documentation within same release cycle

---

*Last Updated: 2024-08-30*
*Version: 1.0.0*