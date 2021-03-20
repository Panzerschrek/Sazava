#pragma once
#include "../Lib/SystemEvent.hpp"
#include <SDL_video.h>


namespace SZV
{

class SystemWindow final
{
public:
	SystemWindow();
	~SystemWindow();

	SystemEvents ProcessEvents();

	InputState GetInputState();

	SDL_Window* GetSDLWindow() const;

private:
	SDL_Window* window_= nullptr;
};

} // namespace SZV
