#include "NewNodeListWidget.hpp"

namespace SZV
{

NewNodeListWidget::NewNodeListWidget(QWidget* const parent, NodeAddCallback node_add_callback)
	: QWidget(parent)
	, node_add_callback_(std::move(node_add_callback))
	, button_box_("box", this)
	, button_sphere_("sphere", this)
{
	connect(&button_box_, &QPushButton::clicked, this, &NewNodeListWidget::OnAddBox);
	connect(&button_sphere_, &QPushButton::clicked, this, &NewNodeListWidget::OnAddSphere);

	layout_.addWidget(&button_box_);
	layout_.addWidget(&button_sphere_);
	setLayout(&layout_);
}

void NewNodeListWidget::OnAddBox()
{
	CSGTree::Cube cube{};
	cube.size= m_Vec3(1.0f, 1.0f, 1.0f);
	node_add_callback_(cube);
}

void NewNodeListWidget::OnAddSphere()
{
	CSGTree::Sphere sphere{};
	sphere.radius= 1.0f;
	node_add_callback_(sphere);
}

} // namespace SZV
