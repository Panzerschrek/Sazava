#include "Host.hpp"
#include "Assert.hpp"
#include <thread>


namespace SZV
{


Host::Host()
	:  system_window_()
	, window_vulkan_(system_window_)
	, init_time_(Clock::now())
	, prev_tick_time_(init_time_)
{
}

bool Host::Loop()
{
	const Clock::time_point tick_start_time= Clock::now();
	prev_tick_time_ = tick_start_time;

	const SystemEvents system_events= system_window_.ProcessEvents();
	for(const SystemEvent& system_event : system_events)
	{
		if(std::get_if<SystemEventTypes::QuitEvent>(&system_event) != nullptr)
			return true;
	}

	const auto command_buffer= window_vulkan_.BeginFrame();
	SZV_UNUSED(command_buffer);

	window_vulkan_.EndFrame(
		{
			[&](const vk::CommandBuffer command_buffer)
			{
				SZV_UNUSED(command_buffer);
			},
			[&](const vk::CommandBuffer command_buffer)
			{
				SZV_UNUSED(command_buffer);
			},
		});

	const Clock::time_point tick_end_time= Clock::now();
	const auto frame_dt= tick_end_time - tick_start_time;

	const float max_fps= 120.0f;

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
