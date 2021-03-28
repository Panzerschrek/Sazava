#include "CSGTreeNodeEditWidget.hpp"
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>

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

void CSGTreeNodeEditWidget::AddValueControl(QGridLayout& layout, float& value, const ValueKind kind, const QString& caption)
{
	const auto value_ptr= &value;

	const auto label= new QLabel(caption);

	const auto box= new QDoubleSpinBox(this);

	switch(kind)
	{
	case ValueKind::Pos:
		box->setSingleStep(0.125f);
		box->setDecimals(3);
		box->setMaximum(+1024.0f);
		box->setMinimum(-1024.0f);
		break;
	case ValueKind::Size:
		box->setSingleStep(0.125f);
		box->setDecimals(3);
		box->setMaximum(+1024.0f);
		box->setMinimum(0.125f);
		break;
	case ValueKind::Angle:
		box->setSingleStep(1.0f);
		box->setDecimals(1);
		box->setMinimum(-180.0f);
		box->setMaximum(+180.0f);
		box->setWrapping(true);
		break;
	}
	box->setValue(*value_ptr);

	connect(
		box,
		static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this,
		[value_ptr](const double value){ *value_ptr= float(value); });

	const int row= layout.rowCount();
	layout.addWidget(label, row, 0);
	layout.addWidget(box, row, 1);
}

void CSGTreeNodeEditWidget::AddPosControl(QGridLayout& layout, m_Vec3& pos)
{
	AddValueControl(layout, pos.x, ValueKind::Pos, "X:");
	AddValueControl(layout, pos.y, ValueKind::Pos, "Y:");
	AddValueControl(layout, pos.z, ValueKind::Pos, "Z:");
}

void CSGTreeNodeEditWidget::AddSizeControl(QGridLayout& layout, m_Vec3& size)
{
	AddValueControl(layout, size.x, ValueKind::Size, "Size X:");
	AddValueControl(layout, size.y, ValueKind::Size, "Size Y:");
	AddValueControl(layout, size.z, ValueKind::Size, "Size Z:");
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

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::Ellipsoid& node)
{
	const auto layout= new QGridLayout(this);
	AddPosControl(*layout, node.center);
	AddSizeControl(*layout, node.size);
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::Cube& node)
{
	const auto layout= new QGridLayout(this);
	AddPosControl(*layout, node.center);
	AddSizeControl(*layout, node.size);
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::Cylinder& node)
{
	const auto layout= new QGridLayout(this);
	AddPosControl(*layout, node.center);
	AddSizeControl(*layout, node.size);
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::Cone& node)
{
	const auto layout= new QGridLayout(this);
	AddPosControl(*layout, node.center);
	AddValueControl(*layout, node.angle, ValueKind::Angle, "Angle:");
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::Paraboloid& node)
{
	const auto layout= new QGridLayout(this);
	AddPosControl(*layout, node.center);
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::Hyperboloid& node)
{
	const auto layout= new QGridLayout(this);
	AddPosControl(*layout, node.center);
	setLayout(layout);
}

void CSGTreeNodeEditWidget::AddWidgets(CSGTree::HyperbolicParaboloid& node)
{
	const auto layout= new QGridLayout(this);
	AddPosControl(*layout, node.center);
	setLayout(layout);
}

} // namespace SZV
