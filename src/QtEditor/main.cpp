#include  "../SDL2ViewerLib/Host.hpp"
#include "CSGTreeModel.hpp"
#include "CSGTreeNodeEditWidget.hpp"
#include "NewNodeListWidget.hpp"
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QWidget>

namespace SZV
{

namespace
{

m_Vec3 GetNodePos(const CSGTree::CSGTreeNode& node);

template<typename T>
m_Vec3 GetNodePosImpl(const T& t){ return t.center; }

m_Vec3 GetNodePosImpl(const CSGTree::MulChain& node)
{
	if(!node.elements.empty())
		return GetNodePos(node.elements.front());
	return m_Vec3(0.0f, 0.0f, 0.0f);
}

m_Vec3 GetNodePosImpl(const CSGTree::AddChain& node)
{
	if(!node.elements.empty())
		return GetNodePos(node.elements.front());
	return m_Vec3(0.0f, 0.0f, 0.0f);
}

m_Vec3 GetNodePosImpl(const CSGTree::SubChain& node)
{
	if(!node.elements.empty())
		return GetNodePos(node.elements.front());
	return m_Vec3(0.0f, 0.0f, 0.0f);
}

m_Vec3 GetNodePos(const CSGTree::CSGTreeNode& node)
{
	return std::visit([](const auto& n){ return GetNodePosImpl(n); }, node);
}

m_Vec3 GetNodeSize(const CSGTree::CSGTreeNode& node);

template<typename T>
m_Vec3 GetNodeSizeImpl(const T& t){ return t.size; }

m_Vec3 GetNodeSizeImpl(const CSGTree::MulChain& node)
{
	if(!node.elements.empty())
		return GetNodeSize(node.elements.front());
	return m_Vec3(1.0f, 1.0f, 1.0f);
}

m_Vec3 GetNodeSizeImpl(const CSGTree::AddChain& node)
{
	if(!node.elements.empty())
		return GetNodeSize(node.elements.front());
	return m_Vec3(1.0f, 1.0f, 1.0f);
}

m_Vec3 GetNodeSizeImpl(const CSGTree::SubChain& node)
{
	if(!node.elements.empty())
		return GetNodeSize(node.elements.front());
	return m_Vec3(1.0f, 1.0f, 1.0f);
}

m_Vec3 GetNodeSizeImpl(const CSGTree::HyperbolicParaboloid&)
{
	return m_Vec3(1.0f, 1.0f, 1.0f);
}

m_Vec3 GetNodeSize(const CSGTree::CSGTreeNode& node)
{
	return std::visit([](const auto& n){ return GetNodeSizeImpl(n); }, node);
}

void SetNodePosImpl(CSGTree::MulChain&, const m_Vec3&){}
void SetNodePosImpl(CSGTree::AddChain&, const m_Vec3&){}
void SetNodePosImpl(CSGTree::SubChain&, const m_Vec3&){}

template<typename T> void SetNodePosImpl(T& node, const m_Vec3& pos){ node.center= pos; }

void SetNodePos(CSGTree::CSGTreeNode& node, const m_Vec3& pos)
{
	std::visit([&](auto& n){ SetNodePosImpl(n, pos); }, node);
}

void SetNodeSizeImpl(CSGTree::MulChain&, const m_Vec3&){}
void SetNodeSizeImpl(CSGTree::AddChain&, const m_Vec3&){}
void SetNodeSizeImpl(CSGTree::SubChain&, const m_Vec3&){}
void SetNodeSizeImpl(CSGTree::HyperbolicParaboloid&, const m_Vec3&){}

template<typename T> void SetNodeSizeImpl(T& node, const m_Vec3& size){ node.size= size; }

void SetNodeSize(CSGTree::CSGTreeNode& node, const m_Vec3& size)
{
	std::visit([&](auto& n){ SetNodeSizeImpl(n, size); }, node);
}

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

	void AddNode(CSGTree::CSGTreeNode node_template)
	{
		const auto index= csg_tree_view_.currentIndex();
		if(!index.isValid())
			return;

		const auto& node= *reinterpret_cast<CSGTree::CSGTreeNode*>(index.internalPointer());
		const m_Vec3 current_pos= GetNodePos(node);
		const m_Vec3 current_size= GetNodeSize(node);

		SetNodePos(node_template, current_pos);
		SetNodeSize(node_template, current_size);
		csg_tree_model_.AddNode(index, std::move(node_template));
	}

private:
	void OnContextMenu(const QPoint& p)
	{
		const auto menu= new QMenu(this);

		menu->addAction("delete", this, &CSGNodesTreeWidget::OnDeleteNode);
		menu->addAction("move up", this, &CSGNodesTreeWidget::OnMoveUpNode);
		menu->addAction("move down", this, &CSGNodesTreeWidget::OnMoveDownNode);

		menu->popup(csg_tree_view_.viewport()->mapToGlobal(p));
	}

	void OnDeleteNode()
	{
		csg_tree_model_.DeleteNode(csg_tree_view_.currentIndex());
	}

	void OnMoveUpNode()
	{
		csg_tree_model_.MoveUpNode(csg_tree_view_.currentIndex());
	}

	void OnMoveDownNode()
	{
		csg_tree_model_.MoveDownNode(csg_tree_view_.currentIndex());
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
		, new_node_list_widget_(
			this,
			[this](CSGTree::CSGTreeNode node){ csg_nodes_tree_widget_.AddNode(std::move(node)); }
			)
		, csg_nodes_tree_widget_(csg_tree_model_, this)
	{
		setMinimumSize(300, 480);

		connect(&timer_, &QTimer::timeout, this, &HostWrapper::Loop);
		timer_.start(20);

		layout_.addWidget(&new_node_list_widget_);
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
	QVBoxLayout layout_;
	NewNodeListWidget new_node_list_widget_;
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
