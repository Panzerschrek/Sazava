#include  "../SDL2ViewerLib/Host.hpp"
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

namespace SZV
{

namespace
{

// Workaround for old Qt without QVulkanWindow. Reuse code from SDL2 Viewer to create Vulkan window.

class HostWrapper final : public QWidget
{
public:

	HostWrapper()
	{
		connect(&timer_, &QTimer::timeout, this, &HostWrapper::Loop);
		timer_.start(20);
	}

private:
	void Loop()
	{
		if(host_.Loop())
			close();
	}

private:
	Host host_;
	QTimer timer_;
};

int Main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	HostWrapper window;
	window.show();

	return app.exec();
}

} // namespace

} // namespace SZV

int main(int argc, char* argv[])
{
	return SZV::Main(argc, argv);
}
