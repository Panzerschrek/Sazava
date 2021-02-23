#include "CSGRenderer.hpp"
#include "CSGExpressionGPU.hpp"

namespace SZV
{

namespace
{

#include "shaders/main.vert.h"
#include "shaders/main.frag.h"

struct Uniforms
{
	m_Mat4 view_matrix;
	float cam_pos[4];
};

} // namespace

CSGRenderer::CSGRenderer(WindowVulkan& window_vulkan)
	: vk_device_(window_vulkan.GetVulkanDevice())
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
					std::size(vk_push_constant_ranges), vk_push_constant_ranges));
	}
	{ // Create pipeline.
		shader_vert_=
			vk_device_.createShaderModuleUnique(
				vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
				sizeof(main_vert),
				main_vert));

		shader_frag_=
			vk_device_.createShaderModuleUnique(
				vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
				sizeof(main_frag),
				main_frag));

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

		const vk::PipelineVertexInputStateCreateInfo vk_pipiline_vertex_input_state_create_info(
			vk::PipelineVertexInputStateCreateFlags(),
			0u, nullptr,
			0u, nullptr);

		const vk::PipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_state_create_info(
			vk::PipelineInputAssemblyStateCreateFlags(),
			vk::PrimitiveTopology::eTriangleList);

		const vk::Extent2D viewport_size= window_vulkan.GetViewportSize();
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
					nullptr,
					&vk_pipeline_color_blend_state_create_info,
					nullptr,
					*pipeline_layout_,
					window_vulkan.GetRenderPass(),
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
}

CSGRenderer::~CSGRenderer()
{
	// Sync before destruction.
	vk_device_.waitIdle();
}

void CSGRenderer::BeginFrame(const vk::CommandBuffer command_buffer)
{
	size_t offset= 0;
	++offset;

	{
		csg_data_buffer_host_[offset]= float(int(ExpressionElementType::Cube));
		++offset;

		ExpressionElements::Cube cube{};
		cube.center[0]= 0.0f;
		cube.center[1]= 2.0f;
		cube.center[2]= 0.0f;
		cube.half_size[0]= 0.95f;
		cube.half_size[1]= 0.9f;
		cube.half_size[2]= 0.85f;

		std::memcpy(csg_data_buffer_host_.data() + offset, &cube, sizeof(cube));
		offset+= sizeof(cube) / sizeof(float);
	}
	{
		csg_data_buffer_host_[offset]= float(int(ExpressionElementType::Sphere));
		++offset;

		ExpressionElements::Sphere sphere{};
		sphere.center[0]= 0.0f;
		sphere.center[1]= 2.0f;
		sphere.center[2]= 0.0f;
		sphere.radius= 1.0f;

		std::memcpy(csg_data_buffer_host_.data() + offset, &sphere, sizeof(sphere));
		offset+= sizeof(sphere) / sizeof(float);
	}
	{
		csg_data_buffer_host_[offset]= float(int(ExpressionElementType::Mul));
		++offset;
	}

	csg_data_buffer_host_[0]= float(offset);

	command_buffer.updateBuffer(
		*csg_data_buffer_gpu_,
		0u,
		offset * sizeof(float),
		csg_data_buffer_host_.data());
}

void CSGRenderer::EndFrame(const CameraController& camera_controller, const vk::CommandBuffer command_buffer)
{
	Uniforms uniforms{};
	// TODO - maybe calculate invertse matrix inside camera controller?
	uniforms.view_matrix= camera_controller.CalculateViewMatrix();
	uniforms.view_matrix.Inverse();

	const m_Vec3 cam_pos= camera_controller.GetCameraPosition();
	uniforms.cam_pos[0]= cam_pos.x;
	uniforms.cam_pos[1]= cam_pos.y;
	uniforms.cam_pos[2]= cam_pos.z;

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

	command_buffer.draw(6u, 1u, 0u, 0u);
}

} // namespace SZV
