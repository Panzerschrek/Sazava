#include "Tonemapper.hpp"
#include "Log.hpp"
#include <cmath>


namespace SZV
{


namespace
{

namespace Shaders
{
#include "shaders/blur.vert.h"
#include "shaders/blur.frag.h"
#include "shaders/tonemapping.vert.h"
#include "shaders/tonemapping.frag.h"
} // namespace Shaders

struct Uniforms
{
	struct
	{
		float mix_factor;
		float padding[3];
	} vertex;

	struct
	{
		float bloom_scale;
		float padding[3];
	} fragment;
};

struct UniformsBloom
{
	float blur_vector[2];
	float padding[2];
};

struct ExposureAccumulateBuffer
{
	float exposure[6];
};

const uint32_t g_tex_uniform_binding= 0u;
const uint32_t g_brightness_tex_uniform_binding= 1u;
const uint32_t g_exposure_accumulate_tex_uniform_binding= 2u;
const uint32_t g_blured_tex_uniform_binding= 3u;

} // namespace

Tonemapper::Tonemapper(I_WindowVulkan& window_vulkan)
	: vk_device_(window_vulkan.GetVulkanDevice())
	, queue_family_index_(window_vulkan.GetQueueFamilyIndex())
{
	const vk::Extent2D viewport_size= window_vulkan.GetViewportSize();
	const vk::PhysicalDeviceMemoryProperties& memory_properties= window_vulkan.GetMemoryProperties();

	// Select color buffer format.
	const vk::Format hdr_color_formats[]
	{
		// 16 bit formats have priority over 32bit formats.
		// formats without alpha have priority over formats with alpha.
		vk::Format::eR16G16B16Sfloat,
		vk::Format::eR16G16B16A16Sfloat,
		vk::Format::eR32G32B32Sfloat,
		vk::Format::eR32G32B32A32Sfloat,
		vk::Format::eB10G11R11UfloatPack32, // Keep it last, 5-6 bit mantissa is too low.
	};
	vk::Format framebuffer_image_format= hdr_color_formats[0];
	for(const vk::Format format_candidate : hdr_color_formats)
	{
		const vk::FormatProperties format_properties=
			window_vulkan.GetPhysicalDevice().getFormatProperties(format_candidate);

		const vk::FormatFeatureFlags required_falgs=
			vk::FormatFeatureFlagBits::eColorAttachment | vk::FormatFeatureFlagBits::eColorAttachmentBlend;
		if((format_properties.optimalTilingFeatures & required_falgs) == required_falgs)
		{
			framebuffer_image_format= format_candidate;
			break;
		}
	}

	// Select depth buffer format.
	// We need at least 32-bit float.
	const vk::Format depth_formats[]
	{
		// Depth formats by priority.
		vk::Format::eD32Sfloat,
		vk::Format::eD32SfloatS8Uint,
	};
	vk::Format framebuffer_depth_format= vk::Format::eD16Unorm;
	for(const vk::Format depth_format_candidate : depth_formats)
	{
		const vk::FormatProperties format_properties=
			window_vulkan.GetPhysicalDevice().getFormatProperties(depth_format_candidate);

		const vk::FormatFeatureFlags required_falgs= vk::FormatFeatureFlagBits::eDepthStencilAttachment;
		if((format_properties.optimalTilingFeatures & required_falgs) == required_falgs)
		{
			framebuffer_depth_format= depth_format_candidate;
			break;
		}
	}

	// Calculate image sizes.
	framebuffer_size_ = viewport_size;

	// Use powert of two sizes, because we needs iterative downsampling and it works properly only for power of two images.
	const vk::Extent2D aux_image_size_log2(
		uint32_t(std::log2(double(framebuffer_size_.width ))) - 1u,
		uint32_t(std::log2(double(framebuffer_size_.height))) - 1u);

	aux_image_size_= vk::Extent2D(
		1u << aux_image_size_log2.width ,
		1u << aux_image_size_log2.height);

	Log::Info("Main framebuffer size: ", framebuffer_size_.width, "x", framebuffer_size_.height);
	Log::Info("Auxilarity images size: ", aux_image_size_.width, "x", aux_image_size_.height);
	Log::Info("Main framebuffer color format: ", vk::to_string(framebuffer_image_format));
	Log::Info("Main framebuffer depth format: ", vk::to_string(framebuffer_depth_format));

	{
		framebuffer_image_=
			vk_device_.createImageUnique(
				vk::ImageCreateInfo(
					vk::ImageCreateFlags(),
					vk::ImageType::e2D,
					framebuffer_image_format,
					vk::Extent3D(framebuffer_size_.width, framebuffer_size_.height, 1u),
					1u,
					1u,
					vk::SampleCountFlagBits::e1,
					vk::ImageTiling::eOptimal,
					vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
					vk::SharingMode::eExclusive,
					0u, nullptr,
					vk::ImageLayout::eUndefined));

		const vk::MemoryRequirements image_memory_requirements= vk_device_.getImageMemoryRequirements(*framebuffer_image_);

		vk::MemoryAllocateInfo vk_memory_allocate_info(image_memory_requirements.size);
		for(uint32_t i= 0u; i < memory_properties.memoryTypeCount; ++i)
		{
			if((image_memory_requirements.memoryTypeBits & (1u << i)) != 0 &&
				(memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) != vk::MemoryPropertyFlags())
				vk_memory_allocate_info.memoryTypeIndex= i;
		}

		framebuffer_image_memory_= vk_device_.allocateMemoryUnique(vk_memory_allocate_info);
		vk_device_.bindImageMemory(*framebuffer_image_, *framebuffer_image_memory_, 0u);

		framebuffer_image_view_=
			vk_device_.createImageViewUnique(
				vk::ImageViewCreateInfo(
					vk::ImageViewCreateFlags(),
					*framebuffer_image_,
					vk::ImageViewType::e2D,
					framebuffer_image_format,
					vk::ComponentMapping(),
					vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 1u)));
	}
	{
		framebuffer_depth_image_=
			vk_device_.createImageUnique(
				vk::ImageCreateInfo(
					vk::ImageCreateFlags(),
					vk::ImageType::e2D,
					framebuffer_depth_format,
					vk::Extent3D(framebuffer_size_.width, framebuffer_size_.height, 1u),
					1u,
					1u,
					vk::SampleCountFlagBits::e1,
					vk::ImageTiling::eOptimal,
					vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
					vk::SharingMode::eExclusive,
					0u, nullptr,
					vk::ImageLayout::eUndefined));

		const vk::MemoryRequirements image_memory_requirements= vk_device_.getImageMemoryRequirements(*framebuffer_depth_image_);

		vk::MemoryAllocateInfo vk_memory_allocate_info(image_memory_requirements.size);
		for(uint32_t i= 0u; i < memory_properties.memoryTypeCount; ++i)
		{
			if((image_memory_requirements.memoryTypeBits & (1u << i)) != 0 &&
				(memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) != vk::MemoryPropertyFlags())
				vk_memory_allocate_info.memoryTypeIndex= i;
		}

		framebuffer_depth_image_memory_= vk_device_.allocateMemoryUnique(vk_memory_allocate_info);
		vk_device_.bindImageMemory(*framebuffer_depth_image_, *framebuffer_depth_image_memory_, 0u);

		framebuffer_depth_image_view_=
			vk_device_.createImageViewUnique(
				vk::ImageViewCreateInfo(
					vk::ImageViewCreateFlags(),
					*framebuffer_depth_image_,
					vk::ImageViewType::e2D,
					framebuffer_depth_format,
					vk::ComponentMapping(),
					vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0u, 1u, 0u, 1u)));
	}

	{ // Create main render pass.
		const vk::AttachmentDescription attachment_description[]
		{
			{
				vk::AttachmentDescriptionFlags(),
				framebuffer_image_format,
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eTransferSrcOptimal,
			},
			{
				vk::AttachmentDescriptionFlags(),
				framebuffer_depth_format,
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eStore,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eDepthStencilAttachmentOptimal,
			},
		};

		const vk::AttachmentReference attachment_reference_color(0u, vk::ImageLayout::eColorAttachmentOptimal);
		const vk::AttachmentReference vk_attachment_reference_depth(1u, vk::ImageLayout::eDepthStencilAttachmentOptimal);

		const vk::SubpassDescription subpass_description(
				vk::SubpassDescriptionFlags(),
				vk::PipelineBindPoint::eGraphics,
				0u, nullptr,
				1u, &attachment_reference_color,
				nullptr,
				&vk_attachment_reference_depth);

		main_pass_=
			vk_device_.createRenderPassUnique(
				vk::RenderPassCreateInfo(
					vk::RenderPassCreateFlags(),
					uint32_t(std::size(attachment_description)), attachment_description,
					1u, &subpass_description));

		const vk::ImageView framebuffer_images[]{ *framebuffer_image_view_, *framebuffer_depth_image_view_ };
		main_pass_framebuffer_=
			vk_device_.createFramebufferUnique(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),
					*main_pass_,
					2u, framebuffer_images,
					framebuffer_size_.width , framebuffer_size_.height, 1u));
	}

	{ // Create brightness calculate image.
		brightness_calculate_image_mip_levels_=
			1u + std::max(aux_image_size_log2.width, aux_image_size_log2.height);

		brightness_calculate_image_=
			vk_device_.createImageUnique(
				vk::ImageCreateInfo(
					vk::ImageCreateFlags(),
					vk::ImageType::e2D,
					framebuffer_image_format,
					vk::Extent3D(aux_image_size_.width, aux_image_size_.height, 1u),
					brightness_calculate_image_mip_levels_,
					1u,
					vk::SampleCountFlagBits::e1,
					vk::ImageTiling::eOptimal,
					vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
					vk::SharingMode::eExclusive,
					0u, nullptr,
					vk::ImageLayout::eUndefined));

		const vk::MemoryRequirements image_memory_requirements= vk_device_.getImageMemoryRequirements(*brightness_calculate_image_);

		vk::MemoryAllocateInfo vk_memory_allocate_info(image_memory_requirements.size);
		for(uint32_t i= 0u; i < memory_properties.memoryTypeCount; ++i)
		{
			if((image_memory_requirements.memoryTypeBits & (1u << i)) != 0 &&
				(memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) != vk::MemoryPropertyFlags())
				vk_memory_allocate_info.memoryTypeIndex= i;
		}

		brightness_calculate_image_memory_= vk_device_.allocateMemoryUnique(vk_memory_allocate_info);
		vk_device_.bindImageMemory(*brightness_calculate_image_, *brightness_calculate_image_memory_, 0u);

		brightness_calculate_image_view_=
			vk_device_.createImageViewUnique(
				vk::ImageViewCreateInfo(
					vk::ImageViewCreateFlags(),
					*brightness_calculate_image_,
					vk::ImageViewType::e2D,
					framebuffer_image_format,
					vk::ComponentMapping(),
					vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0u, brightness_calculate_image_mip_levels_, 0u, 1u)));
	}

	// Create bloom render pass.
	{
		const vk::AttachmentDescription attachment_description(
				vk::AttachmentDescriptionFlags(),
				framebuffer_image_format,
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eShaderReadOnlyOptimal);

		const vk::AttachmentReference attachment_reference(0u, vk::ImageLayout::eColorAttachmentOptimal);

		const vk::SubpassDescription subpass_description(
				vk::SubpassDescriptionFlags(),
				vk::PipelineBindPoint::eGraphics,
				0u, nullptr,
				1u, &attachment_reference,
				nullptr,
				nullptr);

		bloom_render_pass_=
			vk_device_.createRenderPassUnique(
				vk::RenderPassCreateInfo(
					vk::RenderPassCreateFlags(),
					1u, &attachment_description,
					1u, &subpass_description));
	}

	// Create bloom images and framebuffers.
	for(BloomBuffer& bloom_buffer : bloom_buffers_)
	{
		bloom_buffer.image=
			vk_device_.createImageUnique(
				vk::ImageCreateInfo(
					vk::ImageCreateFlags(),
					vk::ImageType::e2D,
					framebuffer_image_format,
					vk::Extent3D(aux_image_size_.width, aux_image_size_.height, 1u),
					1u,
					1u,
					vk::SampleCountFlagBits::e1,
					vk::ImageTiling::eOptimal,
					vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
					vk::SharingMode::eExclusive,
					0u, nullptr,
					vk::ImageLayout::eUndefined));

		const vk::MemoryRequirements image_memory_requirements= vk_device_.getImageMemoryRequirements(*bloom_buffer.image);

		vk::MemoryAllocateInfo vk_memory_allocate_info(image_memory_requirements.size);
		for(uint32_t i= 0u; i < memory_properties.memoryTypeCount; ++i)
		{
			if((image_memory_requirements.memoryTypeBits & (1u << i)) != 0 &&
				(memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) != vk::MemoryPropertyFlags())
				vk_memory_allocate_info.memoryTypeIndex= i;
		}

		bloom_buffer.image_memory= vk_device_.allocateMemoryUnique(vk_memory_allocate_info);
		vk_device_.bindImageMemory(*bloom_buffer.image, *bloom_buffer.image_memory, 0u);

		bloom_buffer.image_view=
			vk_device_.createImageViewUnique(
				vk::ImageViewCreateInfo(
					vk::ImageViewCreateFlags(),
					*bloom_buffer.image,
					vk::ImageViewType::e2D,
					framebuffer_image_format,
					vk::ComponentMapping(),
					vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 1u)));

		bloom_buffer.framebuffer=
			vk_device_.createFramebufferUnique(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),
					*bloom_render_pass_,
					1u, &*bloom_buffer.image_view,
					aux_image_size_.width, aux_image_size_.height, 1u));
	}

	// Create uniforms buffer.
	{
		exposure_accumulate_buffer_=
			vk_device_.createBufferUnique(
				vk::BufferCreateInfo(
					vk::BufferCreateFlags(),
					sizeof(ExposureAccumulateBuffer),
					vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst));

		const vk::MemoryRequirements buffer_memory_requirements= vk_device_.getBufferMemoryRequirements(*exposure_accumulate_buffer_);

		vk::MemoryAllocateInfo memory_allocate_info(buffer_memory_requirements.size);
		for(uint32_t i= 0u; i < memory_properties.memoryTypeCount; ++i)
		{
			if((buffer_memory_requirements.memoryTypeBits & (1u << i)) != 0 &&
				(memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) != vk::MemoryPropertyFlags())
				memory_allocate_info.memoryTypeIndex= i;
		}

		exposure_accumulate_memory_= vk_device_.allocateMemoryUnique(memory_allocate_info);
		vk_device_.bindBufferMemory(*exposure_accumulate_buffer_, *exposure_accumulate_memory_, 0u);
	}

	main_pipeline_= CreateMainPipeline(window_vulkan);
	bloom_pipeline_= CreateBloomPipeline();

	// Create descriptor set pool.
	const vk::DescriptorPoolSize vk_descriptor_pool_sizes[]
	{
		{ vk::DescriptorType::eCombinedImageSampler, 2u * 3u },
		{ vk::DescriptorType::eStorageBuffer, 1u * 3u }
	};
	descriptor_pool_=
		vk_device_.createDescriptorPoolUnique(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
				3u, // max sets.
				uint32_t(std::size(vk_descriptor_pool_sizes)), vk_descriptor_pool_sizes));

	{
		// Create descriptor set.
		main_descriptor_set_=
			std::move(
			vk_device_.allocateDescriptorSetsUnique(
				vk::DescriptorSetAllocateInfo(
					*descriptor_pool_,
					1u, &*main_pipeline_.decriptor_set_layout)).front());

		// Write descriptor set.
		const vk::DescriptorImageInfo descriptor_image_info[]
		{
			{
				vk::Sampler(),
				*framebuffer_image_view_,
				vk::ImageLayout::eShaderReadOnlyOptimal
			},
			{
				vk::Sampler(),
				*brightness_calculate_image_view_,
				vk::ImageLayout::eShaderReadOnlyOptimal
			},
			{
				vk::Sampler(),
				*bloom_buffers_[1].image_view,
				vk::ImageLayout::eShaderReadOnlyOptimal
			},
		};

		const vk::DescriptorBufferInfo descriptor_exposure_accumulate_buffer_info(
			*exposure_accumulate_buffer_,
			0u,
			sizeof(ExposureAccumulateBuffer));

		vk_device_.updateDescriptorSets(
			{
				{
					*main_descriptor_set_,
					g_tex_uniform_binding,
					0u,
					1u,
					vk::DescriptorType::eCombinedImageSampler,
					&descriptor_image_info[0],
					nullptr,
					nullptr
				},
				{
					*main_descriptor_set_,
					g_brightness_tex_uniform_binding,
					0u,
					1u,
					vk::DescriptorType::eCombinedImageSampler,
					&descriptor_image_info[1],
					nullptr,
					nullptr
				},
				{
					*main_descriptor_set_,
					g_exposure_accumulate_tex_uniform_binding,
					0u,
					1u,
					vk::DescriptorType::eStorageBuffer,
					nullptr,
					&descriptor_exposure_accumulate_buffer_info,
					nullptr
				},
				{
					*main_descriptor_set_,
					g_blured_tex_uniform_binding,
					0u,
					1u,
					vk::DescriptorType::eCombinedImageSampler,
					&descriptor_image_info[2],
					nullptr,
					nullptr
				},
			},
			{});
	}

	for(BloomBuffer& bloom_buffer : bloom_buffers_)
	{
		// Create descriptor set.
		bloom_buffer.descriptor_set=
			std::move(
			vk_device_.allocateDescriptorSetsUnique(
				vk::DescriptorSetAllocateInfo(
					*descriptor_pool_,
					1u, &*bloom_pipeline_.decriptor_set_layout)).front());

		// Write descriptor set.
		const vk::DescriptorImageInfo descriptor_image_info(
			vk::Sampler(),
			&bloom_buffer == &bloom_buffers_[0] ? *brightness_calculate_image_view_ : *bloom_buffers_[0].image_view,
			vk::ImageLayout::eShaderReadOnlyOptimal);

		vk_device_.updateDescriptorSets(
			{
				{
					*bloom_buffer.descriptor_set,
					g_tex_uniform_binding,
					0u,
					1u,
					vk::DescriptorType::eCombinedImageSampler,
					&descriptor_image_info,
					nullptr,
					nullptr
				}
			},
			{});
	}
}

Tonemapper::~Tonemapper()
{
	// Sync before destruction.
	vk_device_.waitIdle();
}

vk::Extent2D Tonemapper::GetFramebufferSize() const
{
	return framebuffer_size_;
}

vk::RenderPass Tonemapper::GetMainRenderPass() const
{
	return *main_pass_;
}

void Tonemapper::DoMainPass(const vk::CommandBuffer command_buffer, const std::function<void()>& draw_function)
{
	if(!exposure_buffer_prepared_)
	{
		exposure_buffer_prepared_= true;

		ExposureAccumulateBuffer exposure_accumulate_buffer;
		for(float& exposure : exposure_accumulate_buffer.exposure)
			exposure= 1.0f;

		command_buffer.updateBuffer(
			*exposure_accumulate_buffer_,
			0u,
			sizeof(ExposureAccumulateBuffer),
			&exposure_accumulate_buffer);

		const vk::MemoryBarrier memory_barrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead);
		command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eAllGraphics,
			vk::DependencyFlags(),
			1u, &memory_barrier,
			0u, nullptr,
			0u, nullptr);
	}

	const vk::ClearValue clear_value[]
	{
		{ vk::ClearColorValue(std::array<float,4>{0.2f, 0.1f, 0.1f, 0.5f}) },
		{ vk::ClearDepthStencilValue(1.0f, 0u) },
	};

	command_buffer.beginRenderPass(
		vk::RenderPassBeginInfo(
			*main_pass_,
			*main_pass_framebuffer_,
			vk::Rect2D(vk::Offset2D(0, 0), framebuffer_size_),
			2u, clear_value),
		vk::SubpassContents::eInline);

	draw_function();

	command_buffer.endRenderPass();

	// Wait for finish of main rendering.
	// TODO - check all barrienrs in this function !!!
	command_buffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eTransfer,
		vk::DependencyFlags(),
		{ {vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eTransferRead} },
		{},
		{});

	// Calculate exposure.

	// Transfer layout of brightness image to optimal for tranfer destination.
	{
		const vk::ImageMemoryBarrier image_memory_barrier_dst(
			vk::AccessFlagBits::eTransferWrite,
			vk::AccessFlagBits::eMemoryRead,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
			queue_family_index_,
			queue_family_index_,
			*brightness_calculate_image_,
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0u, brightness_calculate_image_mip_levels_, 0u, 1u));

		command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::DependencyFlags(),
			0u, nullptr,
			0u, nullptr,
			1u, &image_memory_barrier_dst);
	}
	// Blit framebuffer image to bightness image.
	{
		const vk::ImageBlit image_blit(
			vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 1u),
			{
				vk::Offset3D(0, 0, 0),
				vk::Offset3D(framebuffer_size_.width, framebuffer_size_.height, 1),
			},
			vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 1u),
			{
				vk::Offset3D(0, 0, 0),
				vk::Offset3D(aux_image_size_.width, aux_image_size_.height, 1),
			});

		command_buffer.blitImage(
			*framebuffer_image_,
			vk::ImageLayout::eTransferSrcOptimal,
			*brightness_calculate_image_,
			vk::ImageLayout::eTransferDstOptimal,
			1u, &image_blit,
			vk::Filter::eLinear);
	}
	// Calculate mip levels of brightness texture.
	for(uint32_t i= 1u; i < brightness_calculate_image_mip_levels_; ++i)
	{
		// Transform previous mip level layout from dst_optimal to src_optimal
		const vk::ImageMemoryBarrier image_memory_barrier_src(
			vk::AccessFlagBits::eTransferWrite,
			vk::AccessFlagBits::eMemoryRead,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eTransferSrcOptimal,
			queue_family_index_,
			queue_family_index_,
			*brightness_calculate_image_,
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, i - 1u, 1u, 0u, 1u));

		command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::DependencyFlags(),
			0u, nullptr,
			0u, nullptr,
			1u, &image_memory_barrier_src);

		const vk::ImageBlit image_blit(
			vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i - 1u, 0u, 1u),
			{
				vk::Offset3D(0, 0, 0),
				vk::Offset3D(std::max(1u, aux_image_size_.width  >> (i-1u)), std::max(1u, aux_image_size_.height >> (i-1u)), 1),
			},
			vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, i - 0u, 0u, 1u),
			{
				vk::Offset3D(0, 0, 0),
				vk::Offset3D(std::max(1u, aux_image_size_.width >> i), std::max(1u, aux_image_size_.height >> i), 1),
			});

		command_buffer.blitImage(
			*brightness_calculate_image_,
			vk::ImageLayout::eTransferSrcOptimal,
			*brightness_calculate_image_,
			vk::ImageLayout::eTransferDstOptimal,
			1u, &image_blit,
			vk::Filter::eLinear);
	}
	// Transfer layout of brightness image to shader read optimal.
	for(uint32_t i= 0u; i < brightness_calculate_image_mip_levels_; ++i)
	{
		const vk::ImageMemoryBarrier image_memory_barrier_final(
			vk::AccessFlagBits::eTransferWrite,
			vk::AccessFlagBits::eMemoryRead,
			i + 1u == brightness_calculate_image_mip_levels_ ? vk::ImageLayout::eTransferDstOptimal : vk::ImageLayout::eTransferSrcOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			queue_family_index_,
			queue_family_index_,
			*brightness_calculate_image_,
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, i, 1u, 0u, 1u));

		command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::DependencyFlags(),
			0u, nullptr,
			0u, nullptr,
			1u, &image_memory_barrier_final);
	}
	// Transfer layout of main image to shader read optimal.
	{
		const vk::ImageMemoryBarrier image_memory_barrier_final(
			vk::AccessFlagBits::eTransferWrite,
			vk::AccessFlagBits::eMemoryRead,
			vk::ImageLayout::eTransferSrcOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			queue_family_index_,
			queue_family_index_,
			*framebuffer_image_,
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1u, 0u, 1u));

		command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::DependencyFlags(),
			0u, nullptr,
			0u, nullptr,
			1u, &image_memory_barrier_final);
	}

	// Make blur for bloom.
	{
		for(BloomBuffer& bloom_buffer : bloom_buffers_)
		{
			command_buffer.beginRenderPass(
				vk::RenderPassBeginInfo(
					*bloom_render_pass_,
					*bloom_buffer.framebuffer,
					vk::Rect2D(vk::Offset2D(0, 0), aux_image_size_),
					0u, nullptr),
				vk::SubpassContents::eInline);

			command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *bloom_pipeline_.pipeline);

			command_buffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				*bloom_pipeline_.pipeline_layout,
				0u,
				1u, &*bloom_buffer.descriptor_set,
				0u, nullptr);

			const float bloom_size= 0.03125f;

			UniformsBloom uniforms;
			if(&bloom_buffer == &bloom_buffers_[0])
			{
				uniforms.blur_vector[0]= bloom_size * float(framebuffer_size_.height) / float(framebuffer_size_.width);
				uniforms.blur_vector[1]= 0.0f;
			}
			else
			{
				uniforms.blur_vector[0]= 0.0f;
				uniforms.blur_vector[1]= bloom_size;
			}

			command_buffer.pushConstants(
				*bloom_pipeline_.pipeline_layout,
				vk::ShaderStageFlagBits::eFragment,
				0u,
				sizeof(uniforms),
				&uniforms);

			command_buffer.draw(6u, 1u, 0u, 0u);

			command_buffer.endRenderPass();

			command_buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,
				vk::PipelineStageFlagBits::eFragmentShader,
				vk::DependencyFlags(),
				{ {vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead} },
				{},
				{});

		} // for bloom buffers.
	}
}

void Tonemapper::EndFrame(const vk::CommandBuffer command_buffer)
{
	command_buffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		*main_pipeline_.pipeline_layout,
		0u,
		1u, &*main_descriptor_set_,
		0u, nullptr);

	const float bloom_scale= 0.125f;

	Uniforms uniforms;
	uniforms.fragment.bloom_scale= bloom_scale;

	const float c_exposure_change_speed= 8.0f;
	uniforms.vertex.mix_factor= 1.0f - std::pow(c_exposure_change_speed, -1.0f / 10.0f); // TODO - use here current frequency.
	uniforms.vertex.mix_factor= std::max(0.0001f, std::min(uniforms.vertex.mix_factor, 0.9999f));

	command_buffer.pushConstants(
		*main_pipeline_.pipeline_layout,
		vk::ShaderStageFlagBits::eVertex,
		offsetof(Uniforms, vertex),
		sizeof(uniforms.vertex),
		&uniforms.vertex);

	command_buffer.pushConstants(
		*main_pipeline_.pipeline_layout,
		vk::ShaderStageFlagBits::eFragment,
		offsetof(Uniforms, fragment),
		sizeof(uniforms.fragment),
		&uniforms.fragment);

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *main_pipeline_.pipeline);
	command_buffer.draw(6u, 1u, 0u, 0u);
}

Tonemapper::Pipeline Tonemapper::CreateMainPipeline(I_WindowVulkan& window_vulkan)
{
	const vk::Extent2D& viewport_size= window_vulkan.GetViewportSize();

	Pipeline pipeline;

	// Create shaders
	pipeline.shader_vert=
		vk_device_.createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
			sizeof(Shaders::tonemapping_vert),
			Shaders::tonemapping_vert));

	pipeline.shader_frag=
		vk_device_.createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
			sizeof(Shaders::tonemapping_frag),
			Shaders::tonemapping_frag));

	// Create image sampler
	pipeline.sampler=
		vk_device_.createSamplerUnique(
			vk::SamplerCreateInfo(
				vk::SamplerCreateFlags(),
				vk::Filter::eLinear,
				vk::Filter::eLinear,
				vk::SamplerMipmapMode::eNearest,
				vk::SamplerAddressMode::eClampToEdge,
				vk::SamplerAddressMode::eClampToEdge,
				vk::SamplerAddressMode::eClampToEdge,
				0.0f,
				VK_FALSE,
				0.0f,
				VK_FALSE,
				vk::CompareOp::eNever,
				0.0f,
				16.0f, // max mip level
				vk::BorderColor::eFloatTransparentBlack,
				VK_FALSE));

	// Create pipeline layout
	const vk::DescriptorSetLayoutBinding descriptor_set_layout_bindings[]
	{
		{
			g_tex_uniform_binding,
			vk::DescriptorType::eCombinedImageSampler,
			1u,
			vk::ShaderStageFlagBits::eFragment,
			&*pipeline.sampler,
		},
		{
			g_brightness_tex_uniform_binding,
			vk::DescriptorType::eCombinedImageSampler,
			1u,
			vk::ShaderStageFlagBits::eVertex,
			&*pipeline.sampler,
		},
		{
			g_exposure_accumulate_tex_uniform_binding,
			vk::DescriptorType::eStorageBuffer,
			1u,
			vk::ShaderStageFlagBits::eVertex,
			nullptr,
		},
		{
			g_blured_tex_uniform_binding,
			vk::DescriptorType::eCombinedImageSampler,
			1u,
			vk::ShaderStageFlagBits::eFragment,
			&*pipeline.sampler,
		},
	};

	pipeline.decriptor_set_layout=
		vk_device_.createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),
				uint32_t(std::size(descriptor_set_layout_bindings)), descriptor_set_layout_bindings));

	const vk::PushConstantRange push_constant_ranges[]
	{
		{
			vk::ShaderStageFlagBits::eVertex,
			offsetof(Uniforms, vertex),
			sizeof(Uniforms::vertex)
		},
		{
			vk::ShaderStageFlagBits::eFragment,
			offsetof(Uniforms, fragment),
			sizeof(Uniforms::fragment)
		},
	};

	pipeline.pipeline_layout=
		vk_device_.createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo(
				vk::PipelineLayoutCreateFlags(),
				1u, &*pipeline.decriptor_set_layout,
				uint32_t(std::size(push_constant_ranges)), push_constant_ranges));

	// Create pipeline.
	const vk::PipelineShaderStageCreateInfo shader_stage_create_info[2]
	{
		{
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eVertex,
			*pipeline.shader_vert,
			"main"
		},
		{
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eFragment,
			*pipeline.shader_frag,
			"main"
		},
	};

	const vk::PipelineVertexInputStateCreateInfo pipiline_vertex_input_state_create_info(
		vk::PipelineVertexInputStateCreateFlags(),
		0u, nullptr,
		0u, nullptr);

	const vk::PipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info(
		vk::PipelineInputAssemblyStateCreateFlags(),
		vk::PrimitiveTopology::eTriangleList);

	const vk::Viewport vk_viewport(0.0f, 0.0f, float(viewport_size.width), float(viewport_size.height), 0.0f, 1.0f);
	const vk::Rect2D vk_scissor(vk::Offset2D(0, 0), viewport_size);

	const vk::PipelineViewportStateCreateInfo pipieline_viewport_state_create_info(
		vk::PipelineViewportStateCreateFlags(),
		1u, &vk_viewport,
		1u, &vk_scissor);

	const vk::PipelineRasterizationStateCreateInfo pipilane_rasterization_state_create_info(
		vk::PipelineRasterizationStateCreateFlags(),
		VK_FALSE,
		VK_FALSE,
		vk::PolygonMode::eFill,
		vk::CullModeFlagBits::eNone,
		vk::FrontFace::eCounterClockwise,
		VK_FALSE, 0.0f, 0.0f, 0.0f,
		1.0f);

	const vk::PipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info;

	const vk::PipelineColorBlendAttachmentState pipeline_color_blend_attachment_state(
		VK_FALSE,
		vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
		vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

	const vk::PipelineColorBlendStateCreateInfo vk_pipeline_color_blend_state_create_info(
		vk::PipelineColorBlendStateCreateFlags(),
		VK_FALSE,
		vk::LogicOp::eCopy,
		1u, &pipeline_color_blend_attachment_state);

	pipeline.pipeline=
		vk_device_.createGraphicsPipelineUnique(
			nullptr,
			vk::GraphicsPipelineCreateInfo(
				vk::PipelineCreateFlags(),
				uint32_t(std::size(shader_stage_create_info)), shader_stage_create_info,
				&pipiline_vertex_input_state_create_info,
				&pipeline_input_assembly_state_create_info,
				nullptr,
				&pipieline_viewport_state_create_info,
				&pipilane_rasterization_state_create_info,
				&pipeline_multisample_state_create_info,
				nullptr,
				&vk_pipeline_color_blend_state_create_info,
				nullptr,
				*pipeline.pipeline_layout,
				window_vulkan.GetRenderPass(),
				0u));

	return pipeline;
}

Tonemapper::Pipeline Tonemapper::CreateBloomPipeline()
{
	Pipeline pipeline;

	// Create shaders
	pipeline.shader_vert=
		vk_device_.createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
			sizeof(Shaders::blur_vert),
			Shaders::blur_vert));

	pipeline.shader_frag=
		vk_device_.createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
			sizeof(Shaders::blur_frag),
			Shaders::blur_frag));

	// Create image sampler
	pipeline.sampler=
		vk_device_.createSamplerUnique(
			vk::SamplerCreateInfo(
				vk::SamplerCreateFlags(),
				vk::Filter::eLinear,
				vk::Filter::eLinear,
				vk::SamplerMipmapMode::eNearest,
				vk::SamplerAddressMode::eClampToEdge,
				vk::SamplerAddressMode::eClampToEdge,
				vk::SamplerAddressMode::eClampToEdge,
				0.0f,
				VK_FALSE,
				0.0f,
				VK_FALSE,
				vk::CompareOp::eNever,
				0.0f,
				0.0f,
				vk::BorderColor::eFloatTransparentBlack,
				VK_FALSE));

	// Create pipeline layout
	const vk::DescriptorSetLayoutBinding descriptor_set_layout_bindings[]
	{
		{
			g_tex_uniform_binding,
			vk::DescriptorType::eCombinedImageSampler,
			1u,
			vk::ShaderStageFlagBits::eFragment,
			&*pipeline.sampler,
		},
	};

	pipeline.decriptor_set_layout=
		vk_device_.createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),
				uint32_t(std::size(descriptor_set_layout_bindings)), descriptor_set_layout_bindings));

	const vk::PushConstantRange push_constant_range(
		vk::ShaderStageFlagBits::eFragment,
		0u,
		sizeof(UniformsBloom));

	pipeline.pipeline_layout=
		vk_device_.createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo(
				vk::PipelineLayoutCreateFlags(),
				1u, &*pipeline.decriptor_set_layout,
				1u, &push_constant_range));

	// Create pipeline.
	const vk::PipelineShaderStageCreateInfo shader_stage_create_info[2]
	{
		{
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eVertex,
			*pipeline.shader_vert,
			"main"
		},
		{
			vk::PipelineShaderStageCreateFlags(),
			vk::ShaderStageFlagBits::eFragment,
			*pipeline.shader_frag,
			"main"
		},
	};

	const vk::PipelineVertexInputStateCreateInfo pipiline_vertex_input_state_create_info(
		vk::PipelineVertexInputStateCreateFlags(),
		0u, nullptr,
		0u, nullptr);

	const vk::PipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info(
		vk::PipelineInputAssemblyStateCreateFlags(),
		vk::PrimitiveTopology::eTriangleList);

	const vk::Viewport vk_viewport(0.0f, 0.0f, float(aux_image_size_.width), float(aux_image_size_.height), 0.0f, 1.0f);
	const vk::Rect2D vk_scissor(vk::Offset2D(0, 0), aux_image_size_);

	const vk::PipelineViewportStateCreateInfo pipieline_viewport_state_create_info(
		vk::PipelineViewportStateCreateFlags(),
		1u, &vk_viewport,
		1u, &vk_scissor);

	const vk::PipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info(
		vk::PipelineRasterizationStateCreateFlags(),
		VK_FALSE,
		VK_FALSE,
		vk::PolygonMode::eFill,
		vk::CullModeFlagBits::eNone,
		vk::FrontFace::eCounterClockwise,
		VK_FALSE, 0.0f, 0.0f, 0.0f,
		1.0f);

	const vk::PipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info;

	const vk::PipelineColorBlendAttachmentState pipeline_color_blend_attachment_state(
		VK_FALSE,
		vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
		vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

	const vk::PipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info(
		vk::PipelineColorBlendStateCreateFlags(),
		VK_FALSE,
		vk::LogicOp::eCopy,
		1u, &pipeline_color_blend_attachment_state);

	pipeline.pipeline=
		vk_device_.createGraphicsPipelineUnique(
			nullptr,
			vk::GraphicsPipelineCreateInfo(
				vk::PipelineCreateFlags(),
				uint32_t(std::size(shader_stage_create_info)), shader_stage_create_info,
				&pipiline_vertex_input_state_create_info,
				&pipeline_input_assembly_state_create_info,
				nullptr,
				&pipieline_viewport_state_create_info,
				&pipeline_rasterization_state_create_info,
				&pipeline_multisample_state_create_info,
				nullptr,
				&pipeline_color_blend_state_create_info,
				nullptr,
				*pipeline.pipeline_layout,
				*bloom_render_pass_,
				0u));

	return pipeline;
}

} // namespace KK
