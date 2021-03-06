#pragma once
#include "SystemWindow.hpp"
#include "../Lib/I_WindowVulkan.hpp"

namespace SZV
{

class WindowVulkan final : public I_WindowVulkan
{
public:
	using DrawFunction= std::function<void(vk::CommandBuffer)>;
	using DrawFunctions= std::vector<DrawFunction>;

public:
	explicit WindowVulkan(const SystemWindow& system_window);
	~WindowVulkan();

	vk::CommandBuffer BeginFrame();
	void EndFrame(const DrawFunctions& draw_functions);

	vk::Device GetVulkanDevice() const override;
	vk::Extent2D GetViewportSize() const override;
	uint32_t GetQueueFamilyIndex() const override;
	vk::RenderPass GetRenderPass() const override; // Render pass for rendering directly into screen.
	bool HasDepthBuffer() const override;
	vk::PhysicalDeviceMemoryProperties GetMemoryProperties() const override;
	vk::PhysicalDevice GetPhysicalDevice() const;

private:
	struct CommandBufferData
	{
		vk::UniqueCommandBuffer command_buffer;
		vk::UniqueSemaphore image_available_semaphore;
		vk::UniqueSemaphore rendering_finished_semaphore;
		vk::UniqueFence submit_fence;
	};

	struct SwapchainFramebufferData
	{
		vk::Image image;
		vk::UniqueImageView image_view;
		vk::UniqueFramebuffer framebuffer;
	};

private:
	// Keep here order of construction.
	vk::UniqueInstance vk_instance_;
	VkDebugReportCallbackEXT vk_debug_report_callback_= VK_NULL_HANDLE;
	vk::UniqueSurfaceKHR vk_surface_;
	vk::UniqueDevice vk_device_;
	vk::Queue vk_queue_= nullptr;
	uint32_t vk_queue_family_index_= ~0u;
	vk::Extent2D viewport_size_;
	vk::PhysicalDeviceMemoryProperties memory_properties_;
	vk::PhysicalDevice physical_device_;
	vk::UniqueSwapchainKHR vk_swapchain_;

	vk::UniqueRenderPass vk_render_pass_;
	std::vector<SwapchainFramebufferData> framebuffers_; // one framebuffer for each swapchain image.

	vk::UniqueCommandPool vk_command_pool_;

	std::vector<CommandBufferData> command_buffers_;
	const CommandBufferData* current_frame_command_buffer_= nullptr;
	size_t frame_count_= 0u;
};

} // namespace SZV
