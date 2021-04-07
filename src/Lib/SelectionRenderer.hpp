#pragma once
#include "CameraController.hpp"
#include "I_WindowVulkan.hpp"

namespace SZV
{

class SelectionRenderer final
{
public:
	explicit SelectionRenderer(I_WindowVulkan& window_vulkan);
	~SelectionRenderer();

	void EndFrame(
		vk::CommandBuffer command_buffer,
		const CameraController& camera_controller,
		const m_Vec3& bb_center, const m_Vec3& bb_size);

private:

private:
	const vk::Device vk_device_;

	vk::UniqueShaderModule shader_vert_;
	vk::UniqueShaderModule shader_frag_;
	vk::UniqueDescriptorSetLayout descriptor_set_layout_;
	vk::UniquePipelineLayout pipeline_layout_;
	vk::UniquePipeline pipeline_;
};

} // namespace SZV
