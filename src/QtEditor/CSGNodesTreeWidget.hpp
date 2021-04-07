#pragma once
#include "CSGTreeModel.hpp"
#include "CSGTreeNodeEditWidget.hpp"
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QTreeView>

namespace SZV
{

class CSGNodesTreeWidget final : public QWidget
{
	Q_OBJECT

public:
	CSGNodesTreeWidget(CSGTreeModel& csg_tree_model, QWidget*  parent);
	void AddNode(CSGTree::CSGTreeNode node_template);

signals:
	void selectionBoxChanged(const m_Vec3& box_center, const m_Vec3& box_size, const m_Vec3& box_angles_deg);

private:
	void OnContextMenu(const QPoint& p);

	void OnDeleteNode();
	void OnMoveUpNode();
	void OnMoveDownNode();

	void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void OnNodeActivated(const QModelIndex& index);

private:
	QVBoxLayout layout_;
	QTreeView csg_tree_view_;
	CSGTreeModel& csg_tree_model_;
	CSGTreeNodeEditWidget* edit_widget_= nullptr;
};

} // namespace SZV
