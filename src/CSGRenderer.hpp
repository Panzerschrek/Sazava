#pragma once
#include "CameraController.hpp"
#include "CSGExpressionTree.hpp"
#include "Tonemapper.hpp"
#include "WindowVulkan.hpp"

namespace SZV
{

class CSGRenderer final
{
public:
	explicit CSGRenderer(WindowVulkan& window_vulkan);
	~CSGRenderer();

	void BeginFrame(
		vk::CommandBuffer command_buffer,
		const CameraController& camera_controller,
		const CSGTree::CSGTreeNode& csg_tree);
	void EndFrame(vk::CommandBuffer command_buffer);

private:
	void Draw(vk::CommandBuffer command_buffer, const CameraController& camera_controller, size_t index_count);

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

	vk::UniqueBuffer surfaces_data_buffer_gpu_;
	vk::UniqueDeviceMemory surfaces_data_buffer_memory_;
	size_t surfaces_buffer_size_= 0;

	vk::UniqueBuffer expressions_data_buffer_gpu_;
	vk::UniqueDeviceMemory expressions_data_buffer_memory_;
	size_t expressions_buffer_size_= 0;

	size_t vertex_buffer_vertices_= 0;
	vk::UniqueBuffer vertex_buffer_;
	vk::UniqueDeviceMemory vertex_buffer_memory_;

	size_t index_buffer_indeces_= 0;
	vk::UniqueBuffer index_buffer_;
	vk::UniqueDeviceMemory index_buffer_memory_;
};

} // namespace SZV
