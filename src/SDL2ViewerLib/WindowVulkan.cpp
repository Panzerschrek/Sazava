#include "WindowVulkan.hpp"
#include "../Lib/Assert.hpp"
#include "../Lib/Log.hpp"
#include "SystemWindow.hpp"
#include <SDL_vulkan.h>
#include <algorithm>
#include <cstring>

namespace SZV
{

namespace
{

vk::PhysicalDeviceFeatures GetRequiredDeviceFeatures()
{
	// http://vulkan.gpuinfo.org/listfeatures.php

	vk::PhysicalDeviceFeatures features;
	features.setVertexPipelineStoresAndAtomics(VK_TRUE); // For tonemapping, 99.7%
	return features;
}

std::string VulkanVersionToString(const uint32_t version)
{
	return
		std::to_string(version >> 22u) + "." +
		std::to_string((version >> 12u) & ((1u << 10u) - 1u)) + "." +
		std::to_string(version & ((1u << 12u) - 1u));
}

VkBool32 VulkanDebugReportCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT object_type,
	uint64_t  object,
	size_t location,
	int32_t message_code,
	const char* const layer_prefix,
	const char* const message,
	void* user_data)
{
	(void)flags;
	(void)location;
	(void)message_code;
	(void)layer_prefix;
	(void)user_data;

	Log::Warning(
		message, "\n",
		" object= ", object,
		" type= ", vk::to_string(vk::DebugReportObjectTypeEXT(object_type)));

	return VK_FALSE;
}

} // namespace

WindowVulkan::WindowVulkan(const SystemWindow& system_window)
{
	#ifdef DEBUG
	const bool use_debug_extensions_and_layers= true;
	#else
	const bool use_debug_extensions_and_layers= false;
	#endif

	const bool vsync= true;

	// Get vulkan extensiion, needed by SDL.
	unsigned int extension_names_count= 0;
	if( !SDL_Vulkan_GetInstanceExtensions(system_window.GetSDLWindow(), &extension_names_count, nullptr) )
		Log::FatalError("Could not get Vulkan instance extensions");

	std::vector<const char*> extensions_list;
	extensions_list.resize(extension_names_count, nullptr);

	if( !SDL_Vulkan_GetInstanceExtensions(system_window.GetSDLWindow(), &extension_names_count, extensions_list.data()) )
		Log::FatalError("Could not get Vulkan instance extensions");

	if(use_debug_extensions_and_layers)
	{
		extensions_list.push_back("VK_EXT_debug_report");
		++extension_names_count;
	}

	// Create Vulkan instance.
	const vk::ApplicationInfo vk_app_info(
		"Sazava",
		VK_MAKE_VERSION(0, 0, 1),
		"Sazava",
		VK_MAKE_VERSION(0, 0, 1),
		VK_MAKE_VERSION(1, 0, 0));

	vk::InstanceCreateInfo vk_instance_create_info(
		vk::InstanceCreateFlags(),
		&vk_app_info,
		0u, nullptr,
		extension_names_count, extensions_list.data());

	if(use_debug_extensions_and_layers)
	{
		const std::vector<vk::LayerProperties> vk_layer_properties= vk::enumerateInstanceLayerProperties();
		static const char* const possible_validation_layers[]
		{
			"VK_LAYER_LUNARG_core_validation",
			"VK_LAYER_KHRONOS_validation",
		};

		for(const char* const& layer_name : possible_validation_layers)
			for(const vk::LayerProperties& property : vk_layer_properties)
				if(std::strcmp(property.layerName, layer_name) == 0)
				{
					vk_instance_create_info.enabledLayerCount= 1u;
					vk_instance_create_info.ppEnabledLayerNames= &layer_name;
					break;
				}
	}

	vk_instance_= vk::createInstanceUnique(vk_instance_create_info);
	Log::Info("Vulkan instance created");

	if(use_debug_extensions_and_layers)
	{
		if(const auto vkCreateDebugReportCallbackEXT=
			PFN_vkCreateDebugReportCallbackEXT(vk_instance_->getProcAddr("vkCreateDebugReportCallbackEXT")))
		{
			const vk::DebugReportCallbackCreateInfoEXT debug_report_callback_create_info(
				vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::eError,
				VulkanDebugReportCallback);

			vkCreateDebugReportCallbackEXT(
				*vk_instance_,
				&static_cast<const VkDebugReportCallbackCreateInfoEXT&>(debug_report_callback_create_info),
				nullptr,
				&vk_debug_report_callback_);
			if(vk_debug_report_callback_ != VK_NULL_HANDLE)
				Log::Info("Vulkan debug callback installed");
		}
	}

	// Create surface.
	VkSurfaceKHR vk_tmp_surface;
	if(!SDL_Vulkan_CreateSurface(system_window.GetSDLWindow(), *vk_instance_, &vk_tmp_surface))
		Log::FatalError("Could not create Vulkan surface");
	vk_surface_= vk::UniqueSurfaceKHR(vk_tmp_surface, vk::ObjectDestroy<vk::Instance>(*vk_instance_));

	SDL_Vulkan_GetDrawableSize(system_window.GetSDLWindow(), reinterpret_cast<int*>(&viewport_size_.width), reinterpret_cast<int*>(&viewport_size_.height));

	// Create physical device. Prefer usage of discrete GPU. TODO - allow user to select device.
	const std::vector<vk::PhysicalDevice> physical_devices= vk_instance_->enumeratePhysicalDevices();
	vk::PhysicalDevice physical_device= physical_devices.front();
	if(physical_devices.size() > 1u)
	{
		for(const vk::PhysicalDevice& physical_device_candidate : physical_devices)
		{
			const vk::PhysicalDeviceProperties properties= physical_device_candidate.getProperties();
			if(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				physical_device= physical_device_candidate;
				break;
			}
		}
	}

	{
		const vk::PhysicalDeviceProperties properties= physical_device.getProperties();
		Log::Info("");
		Log::Info("Vulkan physical device selected");
		Log::Info("API version: ", VulkanVersionToString(properties.apiVersion));
		Log::Info("Driver version: ", VulkanVersionToString(properties.driverVersion));
		Log::Info("Vendor ID: ", properties.vendorID);
		Log::Info("Device ID: ", properties.deviceID);
		Log::Info("Device type: ", vk::to_string(properties.deviceType));
		Log::Info("Device name: ", properties.deviceName);
		Log::Info("");
	}

	// Select queue family.
	const std::vector<vk::QueueFamilyProperties> queue_family_properties= physical_device.getQueueFamilyProperties();
	uint32_t queue_family_index= ~0u;
	for(uint32_t i= 0u; i < queue_family_properties.size(); ++i)
	{
		const VkBool32 supported= physical_device.getSurfaceSupportKHR(i, *vk_surface_);
		if(supported != 0 &&
			queue_family_properties[i].queueCount > 0 &&
			(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits(0))
		{
			queue_family_index= i;
			break;
		}
	}

	if(queue_family_index == ~0u)
		Log::FatalError("Could not select queue family index");

	vk_queue_family_index_= queue_family_index;

	const float queue_priority= 1.0f;
	const vk::DeviceQueueCreateInfo vk_device_queue_create_info(
		vk::DeviceQueueCreateFlags(),
		queue_family_index,
		1u, &queue_priority);

	const char* const device_extension_names[]{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	const vk::PhysicalDeviceFeatures physical_device_features= GetRequiredDeviceFeatures();

	const vk::DeviceCreateInfo vk_device_create_info(
		vk::DeviceCreateFlags(),
		1u, &vk_device_queue_create_info,
		0u, nullptr,
		uint32_t(std::size(device_extension_names)), device_extension_names,
		&physical_device_features);

	// Create physical device.
	// HACK! createDeviceUnique works wrong! Use other method instead.
	//vk_device_= physical_device.createDeviceUnique(vk_device_create_info);
	vk::Device vk_device_tmp;
	if((physical_device.createDevice(&vk_device_create_info, nullptr, &vk_device_tmp)) != vk::Result::eSuccess)
		Log::FatalError("Could not create Vulkan device");
	vk_device_.reset(vk_device_tmp);
	Log::Info("Vulkan logical device created");

	vk_queue_= vk_device_->getQueue(queue_family_index, 0u);

	// Select surface format. Prefer usage of normalized rbga32.
	const std::vector<vk::SurfaceFormatKHR> surface_formats= physical_device.getSurfaceFormatsKHR(*vk_surface_);
	vk::SurfaceFormatKHR surface_format= surface_formats.back();
	for(const vk::SurfaceFormatKHR& surface_format_variant : surface_formats)
	{
		if( surface_format_variant.format == vk::Format::eR8G8B8A8Unorm ||
			surface_format_variant.format == vk::Format::eB8G8R8A8Unorm)
		{
			surface_format= surface_format_variant;
			break;
		}
	}
	Log::Info("Swapchan surface format: ", vk::to_string(surface_format.format), " ", vk::to_string(surface_format.colorSpace));

	// Select present mode. Prefer usage of tripple buffering, than double buffering.
	const std::vector<vk::PresentModeKHR> present_modes= physical_device.getSurfacePresentModesKHR(*vk_surface_);
	vk::PresentModeKHR present_mode= present_modes.front();
	if(vsync)
	{
		if(std::find(present_modes.begin(), present_modes.end(), vk::PresentModeKHR::eFifo) != present_modes.end())
			present_mode= vk::PresentModeKHR::eFifo;
	}
	else
	{
		if(std::find(present_modes.begin(), present_modes.end(), vk::PresentModeKHR::eMailbox) != present_modes.end())
			present_mode= vk::PresentModeKHR::eMailbox;
	}
	Log::Info("Present mode: ", vk::to_string(present_mode));

	const vk::SurfaceCapabilitiesKHR surface_capabilities= physical_device.getSurfaceCapabilitiesKHR(*vk_surface_);

	vk_swapchain_= vk_device_->createSwapchainKHRUnique(
		vk::SwapchainCreateInfoKHR(
			vk::SwapchainCreateFlagsKHR(),
			*vk_surface_,
			surface_capabilities.minImageCount,
			surface_format.format,
			surface_format.colorSpace,
			surface_capabilities.maxImageExtent,
			1u,
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive,
			1u, &queue_family_index,
			vk::SurfaceTransformFlagBitsKHR::eIdentity,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			present_mode));

	// Create render pass and framebuffers for drawing into screen.

	const vk::AttachmentDescription vk_attachment_description(
		vk::AttachmentDescriptionFlags(),
		surface_format.format,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::ePresentSrcKHR);

	const vk::AttachmentReference vk_attachment_reference(
		0u,
		vk::ImageLayout::eColorAttachmentOptimal);

	const vk::SubpassDescription vk_subpass_description(
		vk::SubpassDescriptionFlags(),
		vk::PipelineBindPoint::eGraphics,
		0u, nullptr,
		1u, &vk_attachment_reference);

	vk_render_pass_=
		vk_device_->createRenderPassUnique(
			vk::RenderPassCreateInfo(
				vk::RenderPassCreateFlags(),
				1u, &vk_attachment_description,
				1u, &vk_subpass_description));

	const std::vector<vk::Image> swapchain_images= vk_device_->getSwapchainImagesKHR(*vk_swapchain_);
	framebuffers_.resize(swapchain_images.size());
	for(size_t i= 0u; i < framebuffers_.size(); ++i)
	{
		framebuffers_[i].image= swapchain_images[i];
		framebuffers_[i].image_view=
			vk_device_->createImageViewUnique(
				vk::ImageViewCreateInfo(
					vk::ImageViewCreateFlags(),
					framebuffers_[i].image,
					vk::ImageViewType::e2D,
					surface_format.format,
					vk::ComponentMapping(),
					vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 1u)));
		framebuffers_[i].framebuffer=
			vk_device_->createFramebufferUnique(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),
					*vk_render_pass_,
					1u, &*framebuffers_[i].image_view,
					viewport_size_.width , viewport_size_.height, 1u));
	}

	// Create command pull.
	vk_command_pool_= vk_device_->createCommandPoolUnique(
		vk::CommandPoolCreateInfo(
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			queue_family_index));

	// Create command buffers and it's synchronization primitives.
	command_buffers_.resize(3u); // Use tripple buffering for command buffers.
	for(CommandBufferData& frame_data : command_buffers_)
	{
		frame_data.command_buffer=
			std::move(
			vk_device_->allocateCommandBuffersUnique(
				vk::CommandBufferAllocateInfo(
					*vk_command_pool_,
					vk::CommandBufferLevel::ePrimary,
					1u)).front());

		frame_data.image_available_semaphore= vk_device_->createSemaphoreUnique(vk::SemaphoreCreateInfo());
		frame_data.rendering_finished_semaphore= vk_device_->createSemaphoreUnique(vk::SemaphoreCreateInfo());
		frame_data.submit_fence= vk_device_->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
	}

	memory_properties_= physical_device.getMemoryProperties();
	physical_device_= physical_device;
}

WindowVulkan::~WindowVulkan()
{
	Log::Info("Vulkan deinitialization");

	// Sync before destruction.
	vk_device_->waitIdle();

	if(vk_debug_report_callback_ != VK_NULL_HANDLE)
	{
		if(const auto vkDestroyDebugReportCallbackEXT=
			PFN_vkDestroyDebugReportCallbackEXT(vk_instance_->getProcAddr("vkDestroyDebugReportCallbackEXT")))
			vkDestroyDebugReportCallbackEXT(*vk_instance_, vk_debug_report_callback_, nullptr);
	}
}

vk::CommandBuffer WindowVulkan::BeginFrame()
{
	current_frame_command_buffer_= &command_buffers_[frame_count_ % command_buffers_.size()];
	++frame_count_;

	vk_device_->waitForFences(
		1u, &*current_frame_command_buffer_->submit_fence,
		VK_TRUE,
		std::numeric_limits<uint64_t>::max());

	vk_device_->resetFences(1u, &*current_frame_command_buffer_->submit_fence);

	const vk::CommandBuffer command_buffer= *current_frame_command_buffer_->command_buffer;
	command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	return command_buffer;
}

void WindowVulkan::EndFrame(const DrawFunctions& draw_functions)
{
	const vk::CommandBuffer command_buffer= *current_frame_command_buffer_->command_buffer;

	// Get next swapchain image.
	const uint32_t swapchain_image_index=
		vk_device_->acquireNextImageKHR(
			*vk_swapchain_,
			std::numeric_limits<uint64_t>::max(),
			*current_frame_command_buffer_->image_available_semaphore,
			vk::Fence()).value;

	// Begin render pass.
	command_buffer.beginRenderPass(
		vk::RenderPassBeginInfo(
			*vk_render_pass_,
			*framebuffers_[swapchain_image_index].framebuffer,
			vk::Rect2D(vk::Offset2D(0, 0), viewport_size_),
			0u, nullptr),
		vk::SubpassContents::eInline);

	// Draw into framebuffer.
	for(const DrawFunction& draw_function : draw_functions)
	{
		draw_function(command_buffer);
	}

	// End render pass.
	command_buffer.endRenderPass();

	// End command buffer.
	command_buffer.end();

	// Submit command buffer.
	const vk::PipelineStageFlags wait_dst_stage_mask= vk::PipelineStageFlagBits::eColorAttachmentOutput;
	const vk::SubmitInfo vk_submit_info(
		1u, &*current_frame_command_buffer_->image_available_semaphore,
		&wait_dst_stage_mask,
		1u, &command_buffer,
		1u, &*current_frame_command_buffer_->rendering_finished_semaphore);
	vk_queue_.submit(vk_submit_info, *current_frame_command_buffer_->submit_fence);

	// Present queue.
	vk_queue_.presentKHR(
		vk::PresentInfoKHR(
			1u, &*current_frame_command_buffer_->rendering_finished_semaphore,
			1u, &*vk_swapchain_,
			&swapchain_image_index,
			nullptr));
}

vk::Device WindowVulkan::GetVulkanDevice() const
{
	return *vk_device_;
}

vk::Extent2D WindowVulkan::GetViewportSize() const
{
	return viewport_size_;
}

uint32_t WindowVulkan::GetQueueFamilyIndex() const
{
	return vk_queue_family_index_;
}

vk::RenderPass WindowVulkan::GetRenderPass() const
{
	return *vk_render_pass_;
}

bool WindowVulkan::HasDepthBuffer() const
{
	return false;
}

vk::PhysicalDeviceMemoryProperties WindowVulkan::GetMemoryProperties() const
{
	return memory_properties_;
}

vk::PhysicalDevice WindowVulkan::GetPhysicalDevice() const
{
	return physical_device_;
}

} // namespace SZV
