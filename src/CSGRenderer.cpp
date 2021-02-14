#include "CSGRenderer.hpp"

namespace SZV
{

namespace
{

#include "shaders/main.vert.h"
#include "shaders/main.frag.h"

struct Uniforms
{
	float dummy[4];
};

} // namespace

CSGRenderer::CSGRenderer(WindowVulkan& window_vulkan)
	: vk_device_(window_vulkan.GetVulkanDevice())
{
	const vk::Extent2D viewport_size= window_vulkan.GetViewportSize();

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

	// Create pipeline layout

	const vk::PushConstantRange vk_push_constant_range(vk::ShaderStageFlagBits::eFragment, 0u, sizeof(Uniforms));

	pipeline_layout_=
		vk_device_.createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo(
				vk::PipelineLayoutCreateFlags(),
				0u, nullptr,
				1u, &vk_push_constant_range));

	// Create pipeline.

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

CSGRenderer::~CSGRenderer()
{
	// Sync before destruction.
	vk_device_.waitIdle();
}

void CSGRenderer::EndFrame(const vk::CommandBuffer command_buffer)
{
	Uniforms uniforms{};

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_);

	command_buffer.pushConstants(
		*pipeline_layout_,
		vk::ShaderStageFlagBits::eFragment,
		0u,
		sizeof(Uniforms),
		&uniforms);

	command_buffer.draw(6u, 1u, 0u, 0u);
}

} // namespace SZV
