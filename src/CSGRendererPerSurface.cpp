#include "CSGRendererPerSurface.hpp"
#include "Assert.hpp"

namespace SZV
{

CSGRendererPerSurface::CSGRendererPerSurface(WindowVulkan& window_vulkan)
	: vk_device_(window_vulkan.GetVulkanDevice())
	, tonemapper_(window_vulkan)
{
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

	tonemapper_.DoMainPass(command_buffer, [this]{});
}

void CSGRendererPerSurface::EndFrame(const CameraController& camera_controller, const vk::CommandBuffer command_buffer)
{
	// TODO
	SZV_UNUSED(camera_controller);
	tonemapper_.EndFrame(command_buffer);
}

} // namespace SZV
