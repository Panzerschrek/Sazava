#include "CSGTreeNodeEditWidget.hpp"
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QDoubleSpinBox>

namespace SZV
{

CSGTreeNodeEditWidget::CSGTreeNodeEditWidget(CSGTree::CSGTreeNode& node, QWidget* const parent)
	: QWidget(parent)
{
	std::visit(
			[&](auto& el)
			{
				AddWidgets(el);
			},
			node);
}

void CSGTreeNodeEditWidget::AddPosControl(QLayout& layout, m_Vec3& pos)
{
	const auto add_control=
	[&](float* const value_ptr)
	{
		const auto box= new QDoubleSpinBox(this);

		box->setValue(*value_ptr);
		box->setSingleStep(0.125f);
		box->setDecimals(3);

		connect(
			box,
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this,
			[value_ptr](const double value){ *value_ptr= float(value); });

		layout.addWidget(box);
	};

	add_control(&pos.x);
	add_control(&pos.y);
	add_control(&pos.z);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::MulChain& node)
{
	//TODO
	(void)node;
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::AddChain& node)
{
	//TODO
	(void)node;
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::SubChain& node)
{
	//TODO
	(void)node;
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::Sphere& node)
{
	const auto layout= new QVBoxLayout(this);
	AddPosControl(*layout, node.center);
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::Ellipsoid& node)
{
	const auto layout= new QVBoxLayout(this);
	AddPosControl(*layout, node.center);
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::Cube& node)
{
	const auto layout= new QVBoxLayout(this);
	AddPosControl(*layout, node.center);
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::Cylinder& node)
{
	const auto layout= new QVBoxLayout(this);
	AddPosControl(*layout, node.center);
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::EllipticCylinder& node)
{
	const auto layout= new QVBoxLayout(this);
	AddPosControl(*layout, node.center);
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::Cone& node)
{
	const auto layout= new QVBoxLayout(this);
	AddPosControl(*layout, node.center);
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::Paraboloid& node)
{
	const auto layout= new QVBoxLayout(this);
	AddPosControl(*layout, node.center);
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::Hyperboloid& node)
{
	const auto layout= new QVBoxLayout(this);
	AddPosControl(*layout, node.center);
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::HyperbolicParaboloid& node)
{
	const auto layout= new QVBoxLayout(this);
	AddPosControl(*layout, node.center);
	setLayout(layout);
}

} // namespace SZV
