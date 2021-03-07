#include "Host.hpp"
#include "Assert.hpp"
#include "CSGRendererPerSurface.hpp"
#include "CSGRendererStraightforward.hpp"
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
	return CSGTree::CSGTreeNode
	{
		CSGTree::AddChain
		{ {
			CSGTree::SubChain
			{ {
				CSGTree::Cube{ { 0.0f, 2.0f, 0.0f }, { 1.9f, 1.8f, 1.7f } },
				CSGTree::Sphere{ { 0.0f, 2.0f, 0.5f }, 1.0f },
				CSGTree::Sphere{ { 0.0f, 2.0f, -0.2f }, 0.5f },
				CSGTree::Cube{ { 0.8f, 2.8f, -0.6f }, { 0.5f, 0.5f, 0.5f } },
			} },
			CSGTree::SubChain
			{ {
				CSGTree::Sphere{ { 0.0f, 2.0f, 3.0f }, 1.0f },
				CSGTree::Cylinder{ { 0.0f, 2.0f, 3.0f }, { 0.0f, std::sqrt(0.5f), std::sqrt(0.5f) }, { 1.0f, 0.0f, 0.0f }, 0.5f, 1.96f },
			} },
			CSGTree::MulChain
			{ {
				CSGTree::Cylinder{ { -4.0f, 2.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 0.75f, 3.0f },
				CSGTree::Cylinder{ { -4.0f, 2.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 0.75f, 3.0f },
			} },
			CSGTree::Cube{ { -3.6f, 2.4f, 1.1f }, { 0.25f, 0.25f, 0.20f } },
			CSGTree::Cylinder{ { 3.0f, 3.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, 0.5f, 10.0f },
			CSGTree::Cube{ { 6.0f, 1.0f, 0.0f }, { 1.5f, 2.5f, 4.0f } },
			CSGTree::SubChain
			{ {
				CSGTree::Cube{ { 0.0f, 7.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },
				CSGTree::Cone{ { 0.0f, 7.0f, 0.0f }, { 0.0f, std::sqrt(0.5f), std::sqrt(0.5f) }, { 1.0f, 0.0f, 0.0f }, 3.1415926535f / 4.0f, 1.0f },
			} },
			CSGTree::SubChain
			{ {
				CSGTree::Cube{ { 1.0f, 5.0f, -1.0f }, { 1.0f, 1.0f, 1.0f } },
				CSGTree::Paraboloid{ { 1.0f, 5.0f, -1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, 0.25f, 1.0f },
				CSGTree::Paraboloid{ { 1.0f, 5.1f, -1.1f }, { 0.0f, std::sqrt(0.5f), -std::sqrt(0.5f) }, { 1.0f, 0.0f, 0.0f }, 0.5f, 1.0f },
			} },
			CSGTree::Paraboloid{ { 2.0f, 6.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, 0.5f, 0.5f },
			CSGTree::Cone{ { 2.5f, 8.0f, -2.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, 3.1415926535f / 5.0f, 2.5f },
		} },
	};
}

} // namespace


Host::Host()
	:  system_window_()
	, window_vulkan_(system_window_)
	, csg_renderer_(std::make_unique<CSGRendererPerSurface>(window_vulkan_))
	, camera_controller_(CalculateAspect(window_vulkan_.GetViewportSize()))
	, init_time_(Clock::now())
	, prev_tick_time_(init_time_)
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
	csg_renderer_->BeginFrame(command_buffer, GetTestCSGTree());

	window_vulkan_.EndFrame(
		{
			[&](const vk::CommandBuffer command_buffer)
			{
				csg_renderer_->EndFrame(camera_controller_, command_buffer);
			},
			[&](const vk::CommandBuffer command_buffer)
			{
				SZV_UNUSED(command_buffer);
			},
		});

	const Clock::time_point tick_end_time= Clock::now();
	const auto frame_dt= tick_end_time - tick_start_time;

	const float max_fps= 10.0f;

	const std::chrono::milliseconds min_frame_duration(uint32_t(1000.0f / max_fps));
	if(frame_dt <= min_frame_duration)
	{
		std::this_thread::sleep_for(min_frame_duration - frame_dt);
	}

	return quit_requested_;
}

void Host::CommandQuit()
{
	quit_requested_= true;
}

} // namespace SZV
