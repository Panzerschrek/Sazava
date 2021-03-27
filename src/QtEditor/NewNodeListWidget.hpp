#pragma once
#include "../Lib/CSGExpressionTree.hpp"
#include <QtWidgets/QPushButton>
#include <QtWidgets/QBoxLayout>
#include <functional>

namespace SZV
{

class NewNodeListWidget final : public QWidget
{
public:
	using NodeAddCallback= std::function<void(CSGTree::CSGTreeNode)>;

	explicit NewNodeListWidget(QWidget* const parent, NodeAddCallback node_add_callback);
private:
	void OnAddBox();
	void OnAddSphere();

private:
	const NodeAddCallback node_add_callback_;
	QPushButton button_box_;
	QPushButton button_sphere_;
	QHBoxLayout layout_;
};

} // namespace SZV
