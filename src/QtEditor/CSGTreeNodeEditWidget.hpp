#pragma once
#include "../Lib/CSGExpressionTree.hpp"
#include <QtWidgets/QWidget>
#include <QtWidgets/QGridLayout>

namespace SZV
{

class CSGTreeNodeEditWidget final : public QWidget
{
public:
	CSGTreeNodeEditWidget(CSGTree::CSGTreeNode& node, QWidget* parent = nullptr);

private:
	enum class ValueKind{ Linear, Angular };

	void AddValueControl(QGridLayout& layout, float& value, ValueKind kind, const QString& caption);
	void AddPosControl(QGridLayout& layout, m_Vec3& pos);
	void AddSizeControl(QGridLayout& layout, m_Vec3& size);

	void AddWidgets(CSGTree::MulChain& node);
	void AddWidgets(CSGTree::AddChain& node);
	void AddWidgets(CSGTree::SubChain& node);
	void AddWidgets(CSGTree::Sphere& node);
	void AddWidgets(CSGTree::Ellipsoid& node);
	void AddWidgets(CSGTree::Cube& node);
	void AddWidgets(CSGTree::Cylinder& node);
	void AddWidgets(CSGTree::EllipticCylinder& node);
	void AddWidgets(CSGTree::Cone& node);
	void AddWidgets(CSGTree::Paraboloid& node);
	void AddWidgets(CSGTree::Hyperboloid& node);
	void AddWidgets(CSGTree::HyperbolicParaboloid& node);
};

} // namespace SZV
