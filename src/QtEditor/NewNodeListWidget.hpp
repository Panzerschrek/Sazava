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
	void OnAddMulChain();
	void OnAddAddChain();
	void OnAddSubChain();
	void OnAddEllipsoid();
	void OnAddBox();
	void OnAddCylinder();
	void OnAddCone();
	void OnAddParaboloid();
	void OnAddHyperboloid();
	void OnAddParabolicCylinder();
	void OnAddHyperbolicCylinder();
	void OnAddHyperbolicParaboloid();

private:
	const NodeAddCallback node_add_callback_;
	QPushButton button_mul_chain_;
	QPushButton button_add_chain_;
	QPushButton button_sub_chain_;
	QPushButton button_ellipsoid_;
	QPushButton button_box_;
	QPushButton button_cylinder_;
	QPushButton button_cone_;
	QPushButton button_paraboloid_;
	QPushButton button_hyperboloid_;
	QPushButton button_parabolic_cylinder_;
	QPushButton button_hyperbolic_cylinder_;
	QPushButton button_hyperbolic_paraboloid_;
	QHBoxLayout layout_;
};

} // namespace SZV
