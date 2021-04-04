#pragma once
#include <vulkan/vulkan.hpp>
#include <functional>


namespace SZV
{

class I_WindowVulkan
{
public:
	~I_WindowVulkan()= default;

	virtual vk::Device GetVulkanDevice() const = 0;
	virtual vk::Extent2D GetViewportSize() const = 0;
	virtual uint32_t GetQueueFamilyIndex() const = 0;
	virtual vk::RenderPass GetRenderPass() const = 0; // Render pass for rendering directly into screen.
	virtual bool HasDepthBuffer() const = 0;
	virtual vk::PhysicalDeviceMemoryProperties GetMemoryProperties() const = 0;
	virtual vk::PhysicalDevice GetPhysicalDevice() const = 0;
};

} // namespace SZV
