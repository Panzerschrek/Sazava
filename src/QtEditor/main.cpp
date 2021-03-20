#include  "../SDL2ViewerLib/Host.hpp"
#include "CSGTreeModel.hpp"
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QWidget>
#include <QtWidgets/QBoxLayout>

namespace SZV
{

namespace
{

// Workaround for old Qt without QVulkanWindow. Reuse code from SDL2 Viewer to create Vulkan window.

class HostWrapper final : public QWidget
{
public:

	HostWrapper()
		: layout_(this), csg_tree_view_(this)
	{
		connect(&timer_, &QTimer::timeout, this, &HostWrapper::Loop);
		timer_.start(20);

		csg_tree_view_.setModel(&csg_tree_model_);
		csg_tree_view_.setSelectionBehavior(QAbstractItemView::SelectRows);
		csg_tree_view_.setSelectionMode(QAbstractItemView::SingleSelection);

		setLayout(&layout_);
		layout_.addWidget(&csg_tree_view_);


		csg_tree_model_.SetTree(
			CSGTree::AddChain
			{ {
				CSGTree::Cube{ { 0.0f, 2.0f, 0.0f }, { 1.9f, 1.8f, 1.7f } },
				CSGTree::Sphere{ { 0.0f, 2.0f, 0.5f }, 1.0f },
				CSGTree::Sphere{ { 0.0f, 2.0f, -0.2f }, 0.5f },
				CSGTree::MulChain
				{ {
						CSGTree::Cylinder{ { -4.0f, 2.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 0.75f, 3.0f },
						CSGTree::Cylinder{ { -4.0f, 2.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 0.75f, 3.0f },
				} },
				CSGTree::Cube{ { 0.8f, 2.8f, -0.6f }, { 0.5f, 0.5f, 0.6f } },
			} } );
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
	QVBoxLayout layout_;
	QTreeView csg_tree_view_;
	CSGTreeModel csg_tree_model_;
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
