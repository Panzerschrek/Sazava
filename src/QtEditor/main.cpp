#include  "../SDL2ViewerLib/Host.hpp"
#include "CSGTreeModel.hpp"
#include "CSGTreeNodeEditWidget.hpp"
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QWidget>

namespace SZV
{

namespace
{

class CSGNodesTreeWidget final : public QWidget
{
public:
	CSGNodesTreeWidget(CSGTreeModel& csg_tree_model, QWidget* const parent)
		: QWidget(parent), csg_tree_model_(csg_tree_model)
	{
		csg_tree_view_.setHeaderHidden(true);
		csg_tree_view_.setModel(&csg_tree_model_);
		csg_tree_view_.setSelectionBehavior(QAbstractItemView::SelectRows);
		csg_tree_view_.setSelectionMode(QAbstractItemView::SingleSelection);
		csg_tree_view_.setContextMenuPolicy(Qt::CustomContextMenu);
		connect(
			&csg_tree_view_,
			&QAbstractItemView::customContextMenuRequested,
			this,
			&CSGNodesTreeWidget::OnContextMenu);

		connect(
			&csg_tree_view_,
			&QAbstractItemView::clicked,
			this,
			&CSGNodesTreeWidget::OnNodeActivated);

		setLayout(&layout_);
		layout_.addWidget(&csg_tree_view_);
	}

private:
	void OnContextMenu(const QPoint& p)
	{
		current_selection_= csg_tree_view_.indexAt(p);

		const auto menu= new QMenu(this);

		menu->addAction("delete", this, &CSGNodesTreeWidget::OnDeleteNode);
		menu->addAction("add", this, &CSGNodesTreeWidget::OnAddNode);
		menu->addAction("move up", this, &CSGNodesTreeWidget::OnMoveUpNode);
		menu->addAction("move down", this, &CSGNodesTreeWidget::OnMoveDownNode);

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

	void OnMoveUpNode()
	{
		csg_tree_model_.MoveUpNode(current_selection_);
	}

	void OnMoveDownNode()
	{
		csg_tree_model_.MoveDownNode(current_selection_);
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
	QVBoxLayout layout_;
	QTreeView csg_tree_view_;
	CSGTreeModel& csg_tree_model_;
	QModelIndex current_selection_;
	CSGTreeNodeEditWidget* edit_widget_= nullptr;
};

// Workaround for old Qt without QVulkanWindow. Reuse code from SDL2 Viewer to create Vulkan window.

class HostWrapper final : public QWidget
{
public:
	explicit HostWrapper(QWidget* const parent)
		: QWidget(parent)
		, csg_tree_model_(host_.GetCSGTree())
		, layout_(this)
		, csg_nodes_tree_widget_(csg_tree_model_, this)
	{
		setMinimumSize(300, 480);

		connect(&timer_, &QTimer::timeout, this, &HostWrapper::Loop);
		timer_.start(20);

		layout_.addWidget(&csg_nodes_tree_widget_);
		setLayout(&layout_);
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
	CSGTreeModel csg_tree_model_;
	QHBoxLayout layout_;
	CSGNodesTreeWidget csg_nodes_tree_widget_;
};

class MainWindow final : public QMainWindow
{
public:
	explicit MainWindow()
		: host_wrapper_(this)
	{
		const auto menu_bar = new QMenuBar(this);
		const auto file_menu = menu_bar->addMenu("file");
		file_menu->addAction("quit", this, &QWidget::close);
		setMenuBar(menu_bar);

		setCentralWidget(&host_wrapper_);
	}

private:
	HostWrapper host_wrapper_;
};

int Main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	MainWindow window;
	window.show();

	return app.exec();
}

} // namespace

} // namespace SZV

int main(int argc, char* argv[])
{
	return SZV::Main(argc, argv);
}
