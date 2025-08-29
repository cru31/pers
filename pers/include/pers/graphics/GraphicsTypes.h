#pragma once

namespace pers {

/**
 * @brief Native graphics API handle types
 * 
 * These are opaque handle types that wrap backend-specific objects.
 * Currently implemented as void* but can be changed to strongly-typed handles in the future.
 */

// Device and adapter handles
typedef void* NativeAdapterHandle;      // WGPUAdapter for WebGPU, VkPhysicalDevice for Vulkan
typedef void* NativeDeviceHandle;       // WGPUDevice for WebGPU, VkDevice for Vulkan

// Command and queue handles  
typedef void* NativeQueueHandle;        // WGPUQueue for WebGPU, VkQueue for Vulkan
typedef void* NativeCommandBufferHandle; // WGPUCommandBuffer for WebGPU, VkCommandBuffer for Vulkan
typedef void* NativeEncoderHandle;      // WGPUCommandEncoder for WebGPU, VkCommandBuffer for Vulkan
typedef void* NativeRenderPassHandle;   // WGPURenderPassEncoder for WebGPU

// Resource handles
typedef void* NativeSwapChainHandle;    // Implementation-specific swap chain handle
typedef void* NativeTextureViewHandle;  // WGPUTextureView for WebGPU, VkImageView for Vulkan
typedef void* NativeBufferHandle;       // WGPUBuffer for WebGPU, VkBuffer for Vulkan
typedef void* NativeTextureHandle;      // WGPUTexture for WebGPU, VkImage for Vulkan
typedef void* NativeSamplerHandle;      // WGPUSampler for WebGPU, VkSampler for Vulkan

// Pipeline and shader handles
typedef void* NativePipelineHandle;     // WGPURenderPipeline for WebGPU, VkPipeline for Vulkan
typedef void* NativeShaderHandle;       // WGPUShaderModule for WebGPU, VkShaderModule for Vulkan

// Surface handle
typedef void* NativeSurfaceHandle;      // WGPUSurface for WebGPU, VkSurfaceKHR for Vulkan

} // namespace pers