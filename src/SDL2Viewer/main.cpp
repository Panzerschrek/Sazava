#include "../Lib/Log.hpp"
#include "../SDL2ViewerLib/Host.hpp"

namespace SZV
{

extern "C" int main()
{
	try
	{
		Host host;
		while(!host.Loop()){}
	}
	catch(const std::exception& ex)
	{
		Log::FatalError("Exception throwed: ", ex.what());
	}
}

} // namespace SZV
