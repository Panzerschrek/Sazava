#pragma once
#include "I_CSGRenderer.hpp"

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
	const vk::Device vk_device_;
};

} // namespace SZV
