#pragma once
#include "I_CSGRenderer.hpp"
#include "Tonemapper.hpp"

namespace SZV
{

class CSGRendererPerSurface final : public I_CSGRenderer
{
public:
	explicit CSGRendererPerSurface(WindowVulkan& window_vulkan);
	~CSGRendererPerSurface();

	void BeginFrame(vk::CommandBuffer command_buffer, const CSGTree::CSGTreeNode& csg_tree) override;
	void EndFrame(const CameraController& camera_controller, vk::CommandBuffer command_buffer) override;

private:
	void Draw(vk::CommandBuffer command_buffer);

private:
	const vk::Device vk_device_;
	Tonemapper tonemapper_;

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

	size_t vertex_buffer_vertices_= 0;
	vk::UniqueBuffer vertex_buffer_;
	vk::UniqueDeviceMemory vertex_buffer_memory_;

	size_t index_buffer_indeces_= 0;
	vk::UniqueBuffer index_buffer_;
	vk::UniqueDeviceMemory index_buffer_memory_;
};

} // namespace SZV
