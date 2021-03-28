#include "Host.hpp"
#include "../Lib/Assert.hpp"
#include <thread>


namespace SZV
{

namespace
{

float CalculateAspect(const vk::Extent2D& viewport_size)
{
	return float(viewport_size.width) / float(viewport_size.height);
}

CSGTree::CSGTreeNode GetTestCSGTree()
{
	return CSGTree::AddChain
	{ {
		CSGTree::Box{ { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
	} };
}

} // namespace


Host::Host()
	:  system_window_()
	, window_vulkan_(system_window_)
	, csg_renderer_(window_vulkan_)
	, camera_controller_(CalculateAspect(window_vulkan_.GetViewportSize()))
	, init_time_(Clock::now())
	, prev_tick_time_(init_time_)
	, csg_tree_(GetTestCSGTree())
{
}

bool Host::Loop()
{
	const Clock::time_point tick_start_time= Clock::now();
	const auto dt= tick_start_time - prev_tick_time_;
	prev_tick_time_ = tick_start_time;

	const SystemEvents system_events= system_window_.ProcessEvents();
	for(const SystemEvent& system_event : system_events)
	{
		if(std::get_if<SystemEventTypes::QuitEvent>(&system_event) != nullptr)
			return true;
	}

	{
		const float dt_s= float(dt.count()) * float(Clock::duration::period::num) / float(Clock::duration::period::den);
		camera_controller_.Update(dt_s, system_window_.GetInputState());
	}

	const auto command_buffer= window_vulkan_.BeginFrame();
	csg_renderer_.BeginFrame(command_buffer, camera_controller_, csg_tree_);

	window_vulkan_.EndFrame(
		{
			[&](const vk::CommandBuffer command_buffer)
			{
				csg_renderer_.EndFrame(command_buffer);
			},
			[&](const vk::CommandBuffer command_buffer)
			{
				SZV_UNUSED(command_buffer);
			},
		});

	const Clock::time_point tick_end_time= Clock::now();
	const auto frame_dt= tick_end_time - tick_start_time;

	const float max_fps= 20.0f;

	const std::chrono::milliseconds min_frame_duration(uint32_t(1000.0f / max_fps));
	if(frame_dt <= min_frame_duration)
	{
		std::this_thread::sleep_for(min_frame_duration - frame_dt);
	}

	return quit_requested_;
}

CSGTree::CSGTreeNode& Host::GetCSGTree()
{
	return csg_tree_;
}

} // namespace SZV
