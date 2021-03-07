#include "CSGRendererPerSurface.hpp"
#include "Assert.hpp"

namespace SZV
{

namespace
{

namespace Shaders
{
#include "shaders/surface.vert.h"
#include "shaders/surface.frag.h"


} // namespace Shaders

struct Uniforms
{
	m_Mat4 view_matrix;
};

struct SurfaceVertex
{
	float pos[3];
	float reserved;
};

using IndexType= uint16_t;

} // namespace

CSGRendererPerSurface::CSGRendererPerSurface(WindowVulkan& window_vulkan)
	: vk_device_(window_vulkan.GetVulkanDevice())
	, tonemapper_(window_vulkan)
{
	const vk::PhysicalDeviceMemoryProperties& memory_properties= window_vulkan.GetMemoryProperties();

	{ // Create data buffer
		const uint32_t buffer_size= 65536u; // Maximum size for vkCmdUpdateBuffer
		csg_data_buffer_host_.resize(buffer_size / sizeof(float), 0.0f);

		csg_data_buffer_gpu_=
			vk_device_.createBufferUnique(
				vk::BufferCreateInfo(
					vk::BufferCreateFlags(),
					buffer_size,
					vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst));

		const vk::MemoryRequirements buffer_memory_requirements=
			vk_device_.getBufferMemoryRequirements(*csg_data_buffer_gpu_);

		vk::MemoryAllocateInfo vk_memory_allocate_info(buffer_memory_requirements.size);
		for(uint32_t i= 0u; i < memory_properties.memoryTypeCount; ++i)
		{
			if((buffer_memory_requirements.memoryTypeBits & (1u << i)) != 0 &&
				(memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) != vk::MemoryPropertyFlags())
				vk_memory_allocate_info.memoryTypeIndex= i;
		}

		csg_data_buffer_memory_= vk_device_.allocateMemoryUnique(vk_memory_allocate_info);
		vk_device_.bindBufferMemory(*csg_data_buffer_gpu_, *csg_data_buffer_memory_, 0u);
	}
	{ // Create descriptor set layout
		const vk::DescriptorSetLayoutBinding descriptor_set_layout_bindings[1]
		{
			{
				0u,
				vk::DescriptorType::eStorageBuffer,
				1u,
				vk::ShaderStageFlagBits::eFragment,
				nullptr,
			},
		};

		descriptor_set_layout_=
			vk_device_.createDescriptorSetLayoutUnique(
					vk::DescriptorSetLayoutCreateInfo(
						vk::DescriptorSetLayoutCreateFlags(),
						uint32_t(std::size(descriptor_set_layout_bindings)), descriptor_set_layout_bindings));
	}
	{ // Create pipeline layout
		const vk::PushConstantRange vk_push_constant_ranges[1]
		{
			{vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0u, sizeof(Uniforms)},
		};

		pipeline_layout_=
			vk_device_.createPipelineLayoutUnique(
				vk::PipelineLayoutCreateInfo(
					vk::PipelineLayoutCreateFlags(),
					1u, &*descriptor_set_layout_,
					uint32_t(std::size(vk_push_constant_ranges)), vk_push_constant_ranges));
	}
	{ // Create pipeline.
		shader_vert_=
			vk_device_.createShaderModuleUnique(
				vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
				sizeof(Shaders::surface_vert),
				Shaders::surface_vert));

		shader_frag_=
			vk_device_.createShaderModuleUnique(
				vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
				sizeof(Shaders::surface_frag),
				Shaders::surface_frag));

		const vk::PipelineShaderStageCreateInfo vk_shader_stage_create_info[]
		{
			{
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eVertex,
				*shader_vert_,
				"main"
			},
			{
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eFragment,
				*shader_frag_,
				"main"
			},
		};

		const vk::VertexInputBindingDescription vk_vertex_input_binding_description(
			0u,
			sizeof(SurfaceVertex),
			vk::VertexInputRate::eVertex);

		const vk::VertexInputAttributeDescription vk_vertex_input_attribute_description[1]
		{
			{0u, 0u, vk::Format::eR32G32B32Sfloat, offsetof(SurfaceVertex, pos)},
		};

		const vk::PipelineVertexInputStateCreateInfo vk_pipiline_vertex_input_state_create_info(
			vk::PipelineVertexInputStateCreateFlags(),
			1u, &vk_vertex_input_binding_description,
			uint32_t(std::size(vk_vertex_input_attribute_description)), vk_vertex_input_attribute_description);

		const vk::PipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_state_create_info(
			vk::PipelineInputAssemblyStateCreateFlags(),
			vk::PrimitiveTopology::eTriangleList);

		const vk::Extent2D viewport_size= tonemapper_.GetFramebufferSize();
		const vk::Viewport vk_viewport(0.0f, 0.0f, float(viewport_size.width), float(viewport_size.height), 0.0f, 1.0f);
		const vk::Rect2D vk_scissor(vk::Offset2D(0, 0), viewport_size);

		const vk::PipelineViewportStateCreateInfo vk_pipieline_viewport_state_create_info(
			vk::PipelineViewportStateCreateFlags(),
			1u, &vk_viewport,
			1u, &vk_scissor);

		const vk::PipelineRasterizationStateCreateInfo vk_pipilane_rasterization_state_create_info(
			vk::PipelineRasterizationStateCreateFlags(),
			VK_FALSE,
			VK_FALSE,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eNone,
			vk::FrontFace::eCounterClockwise,
			VK_FALSE, 0.0f, 0.0f, 0.0f,
			1.0f);

		const vk::PipelineMultisampleStateCreateInfo vk_pipeline_multisample_state_create_info;

		const vk::PipelineDepthStencilStateCreateInfo vk_pipeline_depth_state_create_info(
				vk::PipelineDepthStencilStateCreateFlags(),
				VK_TRUE,
				VK_TRUE,
				vk::CompareOp::eLess,
				VK_FALSE,
				VK_FALSE,
				vk::StencilOpState(),
				vk::StencilOpState(),
				0.0f,
				1.0f);

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

		pipeline_=
			vk_device_.createGraphicsPipelineUnique(
				nullptr,
				vk::GraphicsPipelineCreateInfo(
					vk::PipelineCreateFlags(),
					uint32_t(std::size(vk_shader_stage_create_info)), vk_shader_stage_create_info,
					&vk_pipiline_vertex_input_state_create_info,
					&vk_pipeline_input_assembly_state_create_info,
					nullptr,
					&vk_pipieline_viewport_state_create_info,
					&vk_pipilane_rasterization_state_create_info,
					&vk_pipeline_multisample_state_create_info,
					&vk_pipeline_depth_state_create_info,
					&vk_pipeline_color_blend_state_create_info,
					nullptr,
					*pipeline_layout_,
					tonemapper_.GetMainRenderPass(),
					0u));
	}

	{ // Create descriptor pool
		const vk::DescriptorPoolSize vk_descriptor_pool_sizes[1]
		{
			{
				vk::DescriptorType::eStorageBuffer,
				1u // global storage buffers
			},
		};

		descriptor_pool_=
			vk_device_.createDescriptorPoolUnique(
				vk::DescriptorPoolCreateInfo(
					vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
					1u, // max sets.
					uint32_t(std::size(vk_descriptor_pool_sizes)), vk_descriptor_pool_sizes));
	}
	{ // Create and fill descriptor set
		descriptor_set_=
			std::move(
			vk_device_.allocateDescriptorSetsUnique(
				vk::DescriptorSetAllocateInfo(
					*descriptor_pool_,
					1u, &*descriptor_set_layout_)).front());

		const vk::DescriptorBufferInfo descriptor_buffer_info(
			*csg_data_buffer_gpu_,
			0u,
			sizeof(float) * csg_data_buffer_host_.size());

		vk_device_.updateDescriptorSets(
			{
				{
					*descriptor_set_,
					0u,
					0u,
					1u,
					vk::DescriptorType::eStorageBuffer,
					nullptr,
					&descriptor_buffer_info,
					nullptr,
				},
			},
			{});
	}

	{ // Create vertex buffer.
		vertex_buffer_vertices_= 1024;

		vertex_buffer_=
			vk_device_.createBufferUnique(
				vk::BufferCreateInfo(
					vk::BufferCreateFlags(),
					vertex_buffer_vertices_ * sizeof(SurfaceVertex),
					vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst));

		const vk::MemoryRequirements buffer_memory_requirements= vk_device_.getBufferMemoryRequirements(*vertex_buffer_);

			vk::MemoryAllocateInfo vk_memory_allocate_info(buffer_memory_requirements.size);
			for(uint32_t i= 0u; i < memory_properties.memoryTypeCount; ++i)
			{
				if((buffer_memory_requirements.memoryTypeBits & (1u << i)) != 0 &&
					(memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) != vk::MemoryPropertyFlags())
					vk_memory_allocate_info.memoryTypeIndex= i;
			}

			vertex_buffer_memory_= vk_device_.allocateMemoryUnique(vk_memory_allocate_info);
			vk_device_.bindBufferMemory(*vertex_buffer_, *vertex_buffer_memory_, 0u);
	}
	{ // Create index buffer.
		index_buffer_indeces_= 1024;

		index_buffer_=
				vk_device_.createBufferUnique(
					vk::BufferCreateInfo(
						vk::BufferCreateFlags(),
						index_buffer_indeces_ * sizeof(IndexType),
						vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst));

			const vk::MemoryRequirements buffer_memory_requirements= vk_device_.getBufferMemoryRequirements(*index_buffer_);

			vk::MemoryAllocateInfo vk_memory_allocate_info(buffer_memory_requirements.size);
			for(uint32_t i= 0u; i < memory_properties.memoryTypeCount; ++i)
			{
				if((buffer_memory_requirements.memoryTypeBits & (1u << i)) != 0 &&
					(memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) != vk::MemoryPropertyFlags())
					vk_memory_allocate_info.memoryTypeIndex= i;
			}

			index_buffer_memory_= vk_device_.allocateMemoryUnique(vk_memory_allocate_info);
			vk_device_.bindBufferMemory(*index_buffer_, *index_buffer_memory_, 0u);
	}
}

CSGRendererPerSurface::~CSGRendererPerSurface()
{
	// Sync before destruction.
	vk_device_.waitIdle();
}

void CSGRendererPerSurface::BeginFrame(const vk::CommandBuffer command_buffer, const CSGTree::CSGTreeNode& csg_tree)
{
	// TODO
	SZV_UNUSED(command_buffer);
	SZV_UNUSED(csg_tree);

	const SurfaceVertex test_vertices[]
	{
		{ { 0.5f, -0.5f, 0.0f, }, 0.0f },
		{ { 0.4f, +0.5f, 0.0f, }, 0.0f },
		{ { -0.7f, 0.2f, 0.0f, }, 0.0f },
		{ { -0.7f, -0.7f, 0.0f, }, 0.0f },
	};

	command_buffer.updateBuffer(
		*vertex_buffer_,
		0u,
		vk::DeviceSize(sizeof(test_vertices)),
		test_vertices);

	const IndexType test_indeces[]{ 0, 1, 2,  0, 2, 3 };

	command_buffer.updateBuffer(
		*index_buffer_,
		0u,
		vk::DeviceSize(sizeof(test_indeces)),
		test_indeces);

	tonemapper_.DoMainPass(command_buffer, [&]{Draw(command_buffer);});
}

void CSGRendererPerSurface::EndFrame(const CameraController& camera_controller, const vk::CommandBuffer command_buffer)
{
	// TODO
	SZV_UNUSED(camera_controller);
	tonemapper_.EndFrame(command_buffer);
}

void CSGRendererPerSurface::Draw(const vk::CommandBuffer command_buffer)
{
	Uniforms uniforms{};

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_);

	command_buffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		*pipeline_layout_,
		0u,
		1u, &*descriptor_set_,
		0u, nullptr);

	command_buffer.pushConstants(
		*pipeline_layout_,
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		0u,
		sizeof(uniforms),
		&uniforms);

	const vk::DeviceSize offsets= 0u;
	command_buffer.bindVertexBuffers(0u, 1u, &*vertex_buffer_, &offsets);
		command_buffer.bindIndexBuffer(*index_buffer_, 0u, sizeof(IndexType) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);

	command_buffer.drawIndexed(6u, 1u, 0u, 0u, 0u);
}

} // namespace SZV
