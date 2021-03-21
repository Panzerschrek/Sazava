#include  "../SDL2ViewerLib/Host.hpp"
#include "CSGTreeModel.hpp"
#include "CSGTreeNodeEditWidget.hpp"
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QTreeView>
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
		: layout_(this), csg_tree_view_(this)
	{
		connect(&timer_, &QTimer::timeout, this, &HostWrapper::Loop);
		timer_.start(20);

		csg_tree_view_.setModel(&csg_tree_model_);
		csg_tree_view_.setSelectionBehavior(QAbstractItemView::SelectRows);
		csg_tree_view_.setSelectionMode(QAbstractItemView::SingleSelection);
		csg_tree_view_.setContextMenuPolicy(Qt::CustomContextMenu);
		connect(
			&csg_tree_view_,
			&QAbstractItemView::customContextMenuRequested,
			this,
			&HostWrapper::OnContextMenu);

		connect(
			&csg_tree_view_,
			&QAbstractItemView::clicked,
			this,
			&HostWrapper::OnNodeActivated);

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

	void OnContextMenu(const QPoint& p)
	{
		current_selection_= csg_tree_view_.indexAt(p);

		const auto menu=new QMenu(this);

		const auto delete_action= new QAction("delete", this);
		connect(delete_action, &QAction::triggered, this, &HostWrapper::OnDeleteNode);

		const auto add_action= new QAction("add", this);
		connect(add_action, &QAction::triggered, this, &HostWrapper::OnAddNode);

		menu->addAction(delete_action);
		menu->addAction(add_action);

		menu->popup(csg_tree_view_.viewport()->mapToGlobal(p));
	}

	void OnDeleteNode()
	{
		csg_tree_model_.DeleteNode(current_selection_);
	}

	void OnAddNode()
	{
		CSGTree::Paraboloid node{};
		csg_tree_model_.AddNode(current_selection_, node);
	}

	void OnNodeActivated(const QModelIndex& index)
	{
		if(edit_widget_ != nullptr)
		{
			layout_.removeWidget(edit_widget_);
			delete edit_widget_;
			edit_widget_= nullptr;
		}

		if(!index.isValid())
			return;

		auto node= reinterpret_cast<CSGTree::CSGTreeNode*>(index.internalPointer());
		edit_widget_= new CSGTreeNodeEditWidget(*node, this);
		layout_.addWidget(edit_widget_);
	}

private:
	Host host_;
	QTimer timer_;
	QVBoxLayout layout_;
	QTreeView csg_tree_view_;
	CSGTreeModel csg_tree_model_;
	QModelIndex current_selection_;
	CSGTreeNodeEditWidget* edit_widget_= nullptr;
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
