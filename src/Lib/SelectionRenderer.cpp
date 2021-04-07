#include "SelectionRenderer.hpp"

namespace SZV
{

namespace
{

#include "shaders/selection_box.vert.h"
#include "shaders/selection_box.frag.h"

struct Uniforms
{
	m_Mat4 mat;
};

} // namespace

SelectionRenderer::SelectionRenderer(I_WindowVulkan& window_vulkan)
	: vk_device_(window_vulkan.GetVulkanDevice())
{
	const vk::Extent2D& viewport_size= window_vulkan.GetViewportSize();

	// Create shaders
	shader_vert_=
		vk_device_.createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
			sizeof(selection_box_vert),
			selection_box_vert));

	shader_frag_=
		vk_device_.createShaderModuleUnique(
			vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
			sizeof(selection_box_frag),
			selection_box_frag));

	// Create pipeline layout
	descriptor_set_layout_=
		vk_device_.createDescriptorSetLayoutUnique(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),
				0, nullptr));

	const vk::PushConstantRange push_constant_ranges[1]
	{
		{
			vk::ShaderStageFlagBits::eVertex,
			0,
			sizeof(Uniforms)
		},
	};

	pipeline_layout_=
		vk_device_.createPipelineLayoutUnique(
			vk::PipelineLayoutCreateInfo(
				vk::PipelineLayoutCreateFlags(),
				1u, &*descriptor_set_layout_,
				uint32_t(std::size(push_constant_ranges)), push_constant_ranges));

	// Create pipeline.
	const vk::PipelineShaderStageCreateInfo shader_stage_create_info[2]
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

	const vk::PipelineVertexInputStateCreateInfo pipiline_vertex_input_state_create_info(
		vk::PipelineVertexInputStateCreateFlags(),
		0u, nullptr,
		0u, nullptr);

	const vk::PipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info(
		vk::PipelineInputAssemblyStateCreateFlags(),
		vk::PrimitiveTopology::eLineStrip);

	const vk::Viewport vk_viewport(0.0f, 0.0f, float(viewport_size.width), float(viewport_size.height), 0.0f, 1.0f);
	const vk::Rect2D vk_scissor(vk::Offset2D(0, 0), viewport_size);

	const vk::PipelineViewportStateCreateInfo pipieline_viewport_state_create_info(
		vk::PipelineViewportStateCreateFlags(),
		1u, &vk_viewport,
		1u, &vk_scissor);

	const float line_width= 2.0f;

	const vk::PipelineRasterizationStateCreateInfo pipilane_rasterization_state_create_info(
		vk::PipelineRasterizationStateCreateFlags(),
		VK_FALSE,
		VK_FALSE,
		vk::PolygonMode::eFill,
		vk::CullModeFlagBits::eNone,
		vk::FrontFace::eCounterClockwise,
		VK_FALSE, 0.0f, 0.0f, 0.0f,
		line_width);

	const vk::PipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info;

	const vk::PipelineDepthStencilStateCreateInfo vk_pipeline_depth_state_create_info(
		vk::PipelineDepthStencilStateCreateFlags(),
		VK_FALSE,
		VK_FALSE,
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
				uint32_t(std::size(shader_stage_create_info)), shader_stage_create_info,
				&pipiline_vertex_input_state_create_info,
				&pipeline_input_assembly_state_create_info,
				nullptr,
				&pipieline_viewport_state_create_info,
				&pipilane_rasterization_state_create_info,
				&pipeline_multisample_state_create_info,
				window_vulkan.HasDepthBuffer() ? &vk_pipeline_depth_state_create_info : nullptr,
				&vk_pipeline_color_blend_state_create_info,
				nullptr,
				*pipeline_layout_,
				window_vulkan.GetRenderPass(),
				0u));

}

SelectionRenderer::~SelectionRenderer()
{
	// Sync before destruction.
	vk_device_.waitIdle();
}

void SelectionRenderer::EndFrame(
	const vk::CommandBuffer command_buffer,
	const CameraController& camera_controller,
	const m_Vec3& bb_center, const m_Vec3& bb_size, const m_Vec3& angles_deg)
{
	m_Mat4 scale_mat, rotate_x, rotate_y, rotate_z, translate_mat;
	scale_mat.Scale(bb_size);

	const float deg2rad= 3.1415926535f / 180.0f;
	rotate_x.RotateX(-angles_deg.x * deg2rad);
	rotate_y.RotateY(-angles_deg.y * deg2rad);
	rotate_z.RotateZ(-angles_deg.z * deg2rad);

	translate_mat.Translate(bb_center);

	Uniforms uniforms{};
	uniforms.mat= scale_mat * rotate_z * rotate_y * rotate_x * translate_mat * camera_controller.CalculateFullViewMatrix();

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_);

	command_buffer.pushConstants(
		*pipeline_layout_,
		vk::ShaderStageFlagBits::eVertex,
		0u,
		sizeof(uniforms),
		&uniforms);

	command_buffer.draw(9, 1, 0, 0);
}


} // namespace SZV
