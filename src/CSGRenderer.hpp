#pragma once
#include "WindowVulkan.hpp"

namespace SZV
{

class CSGRenderer
{
public:
	explicit CSGRenderer(WindowVulkan& window_vulkan);
	~CSGRenderer();

	void EndFrame(const vk::CommandBuffer command_buffer);

private:
	const vk::Device vk_device_;

	vk::UniqueShaderModule shader_vert_;
	vk::UniqueShaderModule shader_frag_;
	vk::UniquePipelineLayout pipeline_layout_;
	vk::UniquePipeline pipeline_;
};

} // namespace SZV
