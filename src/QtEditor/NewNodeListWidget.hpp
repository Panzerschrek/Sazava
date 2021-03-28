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
	void OnAddSphere();
	void OnAddEllipsoid();
	void OnAddCube();
	void OnAddCylinder();
	void OnAddEllipticCylinder();
	void OnAddCone();
	void OnAddParaboloid();
	void OnAddHyperboloid();
	void OnAddHyperbolicParaboloid();

private:
	const NodeAddCallback node_add_callback_;
	QPushButton button_sphere_;
	QPushButton button_ellipsoid_;
	QPushButton button_cube_;
	QPushButton button_cylinder_;
	QPushButton button_elliptic_cylinder_;
	QPushButton button_cone_;
	QPushButton button_paraboloid_;
	QPushButton button_hyperboloid_;
	QPushButton button_hyperbolic_paraboloid_;
	QHBoxLayout layout_;
};

} // namespace SZV
