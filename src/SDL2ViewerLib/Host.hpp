#pragma once
#include "../Lib/CameraController.hpp"
#include "../Lib/CSGRenderer.hpp"
#include "SystemWindow.hpp"
#include "WindowVulkan.hpp"
#include <chrono>


namespace SZV
{

class Host final
{
public:
	Host();

	// Returns false on quit
	bool Loop();

	CSGTree::CSGTreeNode& GetCSGTree();

private:
	using Clock= std::chrono::steady_clock;

	SystemWindow system_window_;
	WindowVulkan window_vulkan_;
	CSGRenderer csg_renderer_;
	CameraController camera_controller_;

	const Clock::time_point init_time_;
	Clock::time_point prev_tick_time_;

	CSGTree::CSGTreeNode csg_tree_;

	bool quit_requested_= false;
};

} // namespace SZV
