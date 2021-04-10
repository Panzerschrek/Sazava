#include "CSGNodesTreeWidget.hpp"
#include <QtWidgets/QMenu>

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

m_Vec3 GetNodePosImpl(const CSGTree::AddArray& node)
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

m_Vec3 GetNodeSizeImpl(const CSGTree::AddArray& node)
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

m_Vec3 GetNodeAngles(const CSGTree::CSGTreeNode& node);

template<typename T>
m_Vec3 GetNodeAnglesImpl(const T& t){ return t.angles_deg; }

m_Vec3 GetNodeAnglesImpl(const CSGTree::MulChain& node)
{
	if(!node.elements.empty())
		return GetNodeAngles(node.elements.front());
	return m_Vec3(0.0f, 0.0f, 0.0f);
}

m_Vec3 GetNodeAnglesImpl(const CSGTree::AddChain& node)
{
	if(!node.elements.empty())
		return GetNodeAngles(node.elements.front());
	return m_Vec3(0.0f, 0.0f, 0.0f);
}

m_Vec3 GetNodeAnglesImpl(const CSGTree::SubChain& node)
{
	if(!node.elements.empty())
		return GetNodeAngles(node.elements.front());
	return m_Vec3(0.0f, 0.0f, 0.0f);
}

m_Vec3 GetNodeAngles(const CSGTree::CSGTreeNode& node)
{
	return std::visit([](const auto& n){ return GetNodeAnglesImpl(n); }, node);
}

void SetNodePosImpl(CSGTree::MulChain&, const m_Vec3&){}
void SetNodePosImpl(CSGTree::AddChain&, const m_Vec3&){}
void SetNodePosImpl(CSGTree::SubChain&, const m_Vec3&){}
void SetNodePosImpl(CSGTree::AddArray&, const m_Vec3&){}

template<typename T> void SetNodePosImpl(T& node, const m_Vec3& pos){ node.center= pos; }

void SetNodePos(CSGTree::CSGTreeNode& node, const m_Vec3& pos)
{
	std::visit([&](auto& n){ SetNodePosImpl(n, pos); }, node);
}

void SetNodeSizeImpl(CSGTree::MulChain&, const m_Vec3&){}
void SetNodeSizeImpl(CSGTree::AddChain&, const m_Vec3&){}
void SetNodeSizeImpl(CSGTree::SubChain&, const m_Vec3&){}
void SetNodeSizeImpl(CSGTree::AddArray&, const m_Vec3&){}
void SetNodeSizeImpl(CSGTree::HyperbolicParaboloid&, const m_Vec3&){}

template<typename T> void SetNodeSizeImpl(T& node, const m_Vec3& size){ node.size= size; }

void SetNodeSize(CSGTree::CSGTreeNode& node, const m_Vec3& size)
{
	std::visit([&](auto& n){ SetNodeSizeImpl(n, size); }, node);
}

void SetNodeAnglesImpl(CSGTree::MulChain&, const m_Vec3&){}
void SetNodeAnglesImpl(CSGTree::AddChain&, const m_Vec3&){}
void SetNodeAnglesImpl(CSGTree::SubChain&, const m_Vec3&){}
void SetNodeAnglesImpl(CSGTree::HyperbolicParaboloid&, const m_Vec3&){}

template<typename T> void SetNodeAnglesImpl(T& node, const m_Vec3& angles_deg){ node.angles_deg= angles_deg; }

void SetNodeAngles(CSGTree::CSGTreeNode& node, const m_Vec3& angles_deg)
{
	std::visit([&](auto& n){ SetNodeAnglesImpl(n, angles_deg); }, node);
}

} // namespace

CSGNodesTreeWidget::CSGNodesTreeWidget(CSGTreeModel& csg_tree_model, QWidget* const parent)
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
		csg_tree_view_.selectionModel(),
		&QItemSelectionModel::selectionChanged,
		this,
		&CSGNodesTreeWidget::OnSelectionChanged);

	setLayout(&layout_);
	layout_.addWidget(&csg_tree_view_);
}

void CSGNodesTreeWidget::AddNode(CSGTree::CSGTreeNode node_template)
{
	const auto index= csg_tree_view_.currentIndex();
	if(!index.isValid())
		return;

	const auto& node= *reinterpret_cast<CSGTree::CSGTreeNode*>(index.internalPointer());

	if(node.index() == node_template.index())
		node_template= node; // If new node have same type - copy all params.

	const m_Vec3 current_pos= GetNodePos(node);
	const m_Vec3 current_size= GetNodeSize(node);
	const m_Vec3 current_angles= GetNodeAngles(node);

	// Shift new node relative to old node to shoi it in different location.
	const m_Vec3 pos_shifted= current_pos + current_size;

	SetNodePos(node_template, pos_shifted);
	SetNodeSize(node_template, current_size);
	SetNodeAngles(node_template, current_angles);
	csg_tree_model_.AddNode(index, std::move(node_template));
}

void CSGNodesTreeWidget::OnContextMenu(const QPoint& p)
{
	const auto menu= new QMenu(this);

	menu->addAction("delete", this, &CSGNodesTreeWidget::OnDeleteNode);
	menu->addAction("move up", this, &CSGNodesTreeWidget::OnMoveUpNode);
	menu->addAction("move down", this, &CSGNodesTreeWidget::OnMoveDownNode);

	menu->popup(csg_tree_view_.viewport()->mapToGlobal(p));
}

void CSGNodesTreeWidget::OnDeleteNode()
{
	csg_tree_model_.DeleteNode(csg_tree_view_.currentIndex());
	OnNodeActivated(csg_tree_view_.currentIndex());
}

void CSGNodesTreeWidget::OnMoveUpNode()
{
	csg_tree_model_.MoveUpNode(csg_tree_view_.currentIndex());
	OnNodeActivated(csg_tree_view_.currentIndex());
}

void CSGNodesTreeWidget::OnMoveDownNode()
{
	csg_tree_model_.MoveDownNode(csg_tree_view_.currentIndex());
	OnNodeActivated(csg_tree_view_.currentIndex());
}

void CSGNodesTreeWidget::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	Q_UNUSED(deselected);
	if(!selected.empty())
	{
		const auto range= selected.front();
		if(!range.isEmpty())
			return OnNodeActivated(range.indexes().front());
	}
	OnNodeActivated(QModelIndex());
}

void CSGNodesTreeWidget::OnNodeActivated(const QModelIndex& index)
{
	if(edit_widget_ != nullptr)
	{
		layout_.removeWidget(edit_widget_);
		delete edit_widget_;
		edit_widget_= nullptr;
	}

	if(!index.isValid())
	{
		emit selectionBoxChanged(m_Vec3(0.0f, 0.0f, 0.0f), m_Vec3(0.1f, 0.1f, 0.1f), m_Vec3(0.0f, 0.0f, 0.0f));
		return;
	}

	auto& node= *reinterpret_cast<CSGTree::CSGTreeNode*>(index.internalPointer());
	edit_widget_= new CSGTreeNodeEditWidget(node, this);
	layout_.addWidget(edit_widget_);

	emit selectionBoxChanged(GetNodePos(node), GetNodeSize(node), GetNodeAngles(node));
}


} // namespace SZV
