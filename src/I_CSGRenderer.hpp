#pragma once

#include "CameraController.hpp"
#include "CSGExpressionTree.hpp"
#include "WindowVulkan.hpp"

namespace SZV
{

class I_CSGRenderer
{
public:
	virtual ~I_CSGRenderer()= default;

	virtual void BeginFrame(
		vk::CommandBuffer command_buffer,
		const CameraController& camera_controller,
		const CSGTree::CSGTreeNode& csg_tree) = 0;
	virtual void EndFrame(vk::CommandBuffer command_buffer) = 0;
};

} // namespace SZV
