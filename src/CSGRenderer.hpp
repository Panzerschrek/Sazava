#pragma once
#include "CameraController.hpp"
#include "WindowVulkan.hpp"

namespace SZV
{

class CSGRenderer
{
public:
	explicit CSGRenderer(WindowVulkan& window_vulkan);
	~CSGRenderer();

	void BeginFrame(vk::CommandBuffer command_buffer);
	void EndFrame(const CameraController& camera_controller, vk::CommandBuffer command_buffer);

private:
	const vk::Device vk_device_;

	vk::UniqueShaderModule shader_vert_;
	vk::UniqueShaderModule shader_frag_;
	vk::UniqueDescriptorSetLayout descriptor_set_layout_;
	vk::UniquePipelineLayout pipeline_layout_;
	vk::UniquePipeline pipeline_;

	vk::UniqueDescriptorPool descriptor_pool_;
	vk::UniqueDescriptorSet descriptor_set_;

	std::vector<float> csg_data_buffer_host_;
	vk::UniqueBuffer csg_data_buffer_gpu_;
	vk::UniqueDeviceMemory csg_data_buffer_memory_;
};

} // namespace SZV
