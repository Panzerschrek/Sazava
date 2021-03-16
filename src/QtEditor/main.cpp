#include <QtGui/QGuiApplication>
#include <QtGui/QVulkanWindow>

namespace SZV
{

namespace
{

int Main(int argc, char* argv[])
{
	QGuiApplication app(argc, argv);

	QVulkanWindow window;
	window.show();

	return app.exec();
}

} // namespace

} // namespace SZV

int main(int argc, char* argv[])
{
	return SZV::Main(argc, argv);
}
