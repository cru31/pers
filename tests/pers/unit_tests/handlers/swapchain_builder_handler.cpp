#include "swapchain_builder_handler.h"
#include "pers/graphics/SwapChainDescBuilder.h"
#include "pers/graphics/GraphicsEnumStrings.h"
#include "pers/graphics/GraphicsTypes.h"
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#endif
#include <GLFW/glfw3native.h>
#include <algorithm>
#include <iostream>

namespace pers::tests {

bool SwapChainBuilderNegotiationHandler::initializeDevice() {
        if (_logicalDevice) {
            return true;
        }
        
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // Create window for surface  
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // No OpenGL context
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);  // Show window
        _window = glfwCreateWindow(800, 600, "SwapChain Test", nullptr, nullptr);
        if (!_window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        // Create instance
        if (!_instance) {
            InstanceDesc instanceDesc;
            instanceDesc.applicationName = "SwapChain Test";
            instanceDesc.enableValidation = false;
            _instance = _factory->createInstance(instanceDesc);
            if (!_instance) {
                glfwDestroyWindow(_window);
                glfwTerminate();
                return false;
            }
        }
        
        // Create surface from window
        pers::NativeWindowHandle nativeHandle;
#ifdef _WIN32
        nativeHandle.hwnd = glfwGetWin32Window(_window);
#else
        // Linux/macOS would need different handling
        std::cerr << "Non-Windows platform not yet supported" << std::endl;
        return false;
#endif
        
        _surface = _instance->createSurface(&nativeHandle);
        if (!_surface.isValid()) {
            std::cerr << "Failed to create surface" << std::endl;
            glfwDestroyWindow(_window);
            glfwTerminate();
            return false;
        }
        
        // Get physical device compatible with surface
        if (!_physicalDevice) {
            PhysicalDeviceOptions options;
            options.compatibleSurface = _surface;
            _physicalDevice = _instance->requestPhysicalDevice(options);
            if (!_physicalDevice) {
                glfwDestroyWindow(_window);
                glfwTerminate();
                return false;
            }
        }
        
        // Create logical device
        LogicalDeviceDesc desc;
        desc.enableValidation = false;
        _logicalDevice = _physicalDevice->createLogicalDevice(desc);
        
        if (!_logicalDevice) {
            glfwDestroyWindow(_window);
            glfwTerminate();
            return false;
        }
        
        return true;
    }
    
// Helper function to convert string to TextureFormat
TextureFormat SwapChainBuilderNegotiationHandler::stringToTextureFormat(const std::string& str) {
        // Common formats mapping
        if (str == "BGRA8Unorm") return TextureFormat::BGRA8Unorm;
        if (str == "RGBA8Unorm") return TextureFormat::RGBA8Unorm;
        if (str == "BGRA8UnormSrgb") return TextureFormat::BGRA8UnormSrgb;
        if (str == "RGBA8UnormSrgb") return TextureFormat::RGBA8UnormSrgb;
        if (str == "RGBA32Float") return TextureFormat::RGBA32Float;
        if (str == "RGBA16Float") return TextureFormat::RGBA16Float;
        if (str == "R8Unorm") return TextureFormat::R8Unorm;
        if (str == "RG8Unorm") return TextureFormat::RG8Unorm;
        return TextureFormat::Undefined;
    }

// Helper function to convert string to PresentMode
PresentMode SwapChainBuilderNegotiationHandler::stringToPresentMode(const std::string& str) {
        if (str == "Fifo") return PresentMode::Fifo;
        if (str == "Immediate") return PresentMode::Immediate;
        if (str == "Mailbox") return PresentMode::Mailbox;
        if (str == "FifoRelaxed") return PresentMode::FifoRelaxed;
        return PresentMode::Fifo;
    }

// Helper function to convert string to CompositeAlphaMode
CompositeAlphaMode SwapChainBuilderNegotiationHandler::stringToAlphaMode(const std::string& str) {
        if (str == "Auto") return CompositeAlphaMode::Auto;
        if (str == "Opaque") return CompositeAlphaMode::Opaque;
        if (str == "Premultiplied" || str == "PreMultiplied") return CompositeAlphaMode::Premultiplied;
        if (str == "Unpremultiplied") return CompositeAlphaMode::Unpremultiplied;
        if (str == "Inherit") return CompositeAlphaMode::Inherit;
        if (str == "PostMultiplied") return CompositeAlphaMode::PostMultiplied;
        return CompositeAlphaMode::Opaque;
    }

SwapChainBuilderNegotiationHandler::SwapChainBuilderNegotiationHandler()
    : _factory(std::make_shared<WebGPUBackendFactory>()) {
}

SwapChainBuilderNegotiationHandler::~SwapChainBuilderNegotiationHandler() {
        // Clean up resources
        _swapChain.reset();
        _logicalDevice.reset();
        _physicalDevice.reset();
        _surface = NativeSurfaceHandle();
        _instance.reset();
        
        if (_window) {
            glfwDestroyWindow(_window);
            _window = nullptr;
        }
        glfwTerminate();
    }

std::string SwapChainBuilderNegotiationHandler::getTestType() const {
    return "SwapChain Builder Negotiation";
}

TestResult SwapChainBuilderNegotiationHandler::execute(const TestVariation& variation) {

        setupLogCapture();

        TestResult result;
        
        // Initialize device if needed
        if (!initializeDevice()) {
            result.passed = false;
            result.actualBehavior = "Device initialization failed";
            result.failureReason = "Failed to create device for SwapChain test";
            result.actualProperties["deviceInitialized"] = false;
            return result;
        }
        
        // Parse builder config from variation options
        auto builderWidth = getOption<int>(variation.options, "builder_width", 800);
        auto builderHeight = getOption<int>(variation.options, "builder_height", 600);
        auto preferredFormatStr = getOption<std::string>(variation.options, "preferred_format", "BGRA8Unorm");
        auto preferredPresentModeStr = getOption<std::string>(variation.options, "preferred_present_mode", "Fifo");
        auto preferredAlphaModeStr = getOption<std::string>(variation.options, "preferred_alpha_mode", "Opaque");
        
        SwapChainDescBuilder builder;
        builder.withDimensions(builderWidth, builderHeight);
        
        // Set format with fallbacks
        auto preferredFormat = stringToTextureFormat(preferredFormatStr);
        std::vector<TextureFormat> formatFallbacks;
        // Check if we have format fallbacks in options
        if (variation.options.count("format_fallbacks")) {
            try {
                auto fallbacks = std::any_cast<std::vector<std::string>>(variation.options.at("format_fallbacks"));
                for (const auto& fallback : fallbacks) {
                    formatFallbacks.push_back(stringToTextureFormat(fallback));
                }
            } catch (...) {}
        }
        builder.withFormat(preferredFormat, formatFallbacks);
        
        // Set present mode with fallbacks
        auto preferredPresentMode = stringToPresentMode(preferredPresentModeStr);
        std::vector<PresentMode> presentModeFallbacks;
        if (variation.options.count("present_mode_fallbacks")) {
            try {
                auto fallbacks = std::any_cast<std::vector<std::string>>(variation.options.at("present_mode_fallbacks"));
                for (const auto& fallback : fallbacks) {
                    presentModeFallbacks.push_back(stringToPresentMode(fallback));
                }
            } catch (...) {}
        }
        builder.withPresentMode(preferredPresentMode, presentModeFallbacks);
        
        // Set alpha mode with fallbacks
        auto preferredAlphaMode = stringToAlphaMode(preferredAlphaModeStr);
        std::vector<CompositeAlphaMode> alphaModeFallbacks;
        if (variation.options.count("alpha_mode_fallbacks")) {
            try {
                auto fallbacks = std::any_cast<std::vector<std::string>>(variation.options.at("alpha_mode_fallbacks"));
                for (const auto& fallback : fallbacks) {
                    alphaModeFallbacks.push_back(stringToAlphaMode(fallback));
                }
            } catch (...) {}
        }
        builder.withAlphaMode(preferredAlphaMode, alphaModeFallbacks);
        
        // Create SwapChain with the real surface
        SwapChainDesc swapChainDesc;
        swapChainDesc.width = builderWidth;
        swapChainDesc.height = builderHeight;
        swapChainDesc.format = preferredFormat;
        swapChainDesc.presentMode = preferredPresentMode;
        swapChainDesc.usage = static_cast<TextureUsageFlags>(TextureUsage::RenderAttachment);
        
        _swapChain = _logicalDevice->createSwapChain(_surface, swapChainDesc);
        if (!_swapChain) {
            result.passed = false;
            result.actualBehavior = "Failed to create SwapChain";
            result.failureReason = "SwapChain creation failed";
            result.actualProperties["swapChainCreated"] = false;
            return result;
        }
        
        // Query real surface capabilities from the SwapChain
        SurfaceCapabilities caps = _swapChain->querySurfaceCapabilities(_physicalDevice);
        
        // Process window events to keep window responsive
        glfwPollEvents();
        
        // Log the real capabilities
        std::cout << "Real surface capabilities:" << std::endl;
        std::cout << "  Formats:" << std::endl;
        for (const auto& format : caps.formats) {
            std::cout << "    - " << GraphicsEnumStrings::toString(format) << std::endl;
        }
        std::cout << "  Present Modes:" << std::endl;
        for (const auto& mode : caps.presentModes) {
            std::cout << "    - " << GraphicsEnumStrings::toString(mode) << std::endl;
        }
        std::cout << "  Alpha Modes:" << std::endl;
        for (const auto& mode : caps.alphaModes) {
            std::cout << "    - " << GraphicsEnumStrings::toString(mode) << std::endl;
        }
        std::cout << "  Dimensions: " << caps.minWidth << "x" << caps.minHeight 
                  << " to " << caps.maxWidth << "x" << caps.maxHeight << std::endl;
        
        // Execute negotiation
        auto negotiationResult = builder.negotiate(caps);

        // Transfer captured logs to result
        transferLogsToResult(result);
        
        // Store actual results
        result.actualProperties["format_supported"] = negotiationResult.formatSupported;
        result.actualProperties["present_mode_supported"] = negotiationResult.presentModeSupported;
        result.actualProperties["alpha_mode_supported"] = negotiationResult.alphaModeSupported;
        
        if (negotiationResult.formatSupported) {
            result.actualProperties["negotiated_format"] = GraphicsEnumStrings::toString(negotiationResult.negotiatedFormat);
        }
        if (negotiationResult.presentModeSupported) {
            result.actualProperties["negotiated_present_mode"] = GraphicsEnumStrings::toString(negotiationResult.negotiatedPresentMode);
        }
        if (negotiationResult.alphaModeSupported) {
            result.actualProperties["negotiated_alpha_mode"] = GraphicsEnumStrings::toString(negotiationResult.negotiatedAlphaMode);
        }
        
        if (!negotiationResult.failureReason.empty()) {
            result.actualProperties["failure_reason"] = negotiationResult.failureReason;
        }
        
        // Get negotiation logs
        const auto& logs = builder.getNegotiationLogs();
        std::vector<std::string> logsVec;
        for (const auto& log : logs) {
            logsVec.push_back(log);
        }
        result.actualProperties["negotiation_logs"] = logsVec;
        
        // Determine pass/fail based on expected behavior
        result.passed = true;
        
        // Check if format negotiation matched expectations
        if (variation.expectedBehavior.properties.count("format_supported")) {
            bool expectedSupported = std::any_cast<bool>(variation.expectedBehavior.properties.at("format_supported"));
            if (negotiationResult.formatSupported != expectedSupported) {
                result.passed = false;
                result.failureReason = "Format support mismatch";
            }
        }
        
        // Check if present mode negotiation matched expectations
        if (variation.expectedBehavior.properties.count("present_mode_supported")) {
            bool expectedSupported = std::any_cast<bool>(variation.expectedBehavior.properties.at("present_mode_supported"));
            if (negotiationResult.presentModeSupported != expectedSupported) {
                result.passed = false;
                result.failureReason = "Present mode support mismatch";
            }
        }
        
        // Check if alpha mode negotiation matched expectations
        if (variation.expectedBehavior.properties.count("alpha_mode_supported")) {
            bool expectedSupported = std::any_cast<bool>(variation.expectedBehavior.properties.at("alpha_mode_supported"));
            if (negotiationResult.alphaModeSupported != expectedSupported) {
                result.passed = false;
                result.failureReason = "Alpha mode support mismatch";
            }
        }
        
        // Set actual behavior description
        if (result.passed) {
            result.actualBehavior = "Negotiation completed successfully";
        } else {
            result.actualBehavior = "Negotiation result did not match expectations";
        }

        return result;
}

} // namespace pers::tests