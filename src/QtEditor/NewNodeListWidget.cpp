#include "NewNodeListWidget.hpp"

namespace SZV
{

NewNodeListWidget::NewNodeListWidget(QWidget* const parent, NodeAddCallback node_add_callback)
	: QWidget(parent)
	, node_add_callback_(std::move(node_add_callback))
	, button_mul_chain_("mut chain", this)
	, button_add_chain_("add chain", this)
	, button_sub_chain_("sub chain", this)
	, button_ellipsoid_("ellipsoid", this)
	, button_box_("box", this)
	, button_cylinder_("cylinder", this)
	, button_cone_("cone", this)
	, button_paraboloid_("paraboloid", this)
	, button_hyperboloid_("hyperboloid", this)
	, button_hyperbolic_paraboloid_("hyperbolic paraboloid", this)
{
	connect(&button_mul_chain_, &QPushButton::clicked, this, &NewNodeListWidget::OnAddMulChain);
	connect(&button_add_chain_, &QPushButton::clicked, this, &NewNodeListWidget::OnAddAddChain);
	connect(&button_sub_chain_, &QPushButton::clicked, this, &NewNodeListWidget::OnAddSubChain);
	connect(&button_ellipsoid_, &QPushButton::clicked, this, &NewNodeListWidget::OnAddEllipsoid);
	connect(&button_box_, &QPushButton::clicked, this, &NewNodeListWidget::OnAddBox);
	connect(&button_cylinder_, &QPushButton::clicked, this, &NewNodeListWidget::OnAddCylinder);
	connect(&button_cone_, &QPushButton::clicked, this, &NewNodeListWidget::OnAddCone);
	connect(&button_paraboloid_, &QPushButton::clicked, this, &NewNodeListWidget::OnAddParaboloid);
	connect(&button_hyperboloid_, &QPushButton::clicked, this, &NewNodeListWidget::OnAddHyperboloid);
	connect(&button_hyperbolic_paraboloid_, &QPushButton::clicked, this, &NewNodeListWidget::OnAddHyperbolicParaboloid);

	layout_.addWidget(&button_mul_chain_);
	layout_.addWidget(&button_add_chain_);
	layout_.addWidget(&button_sub_chain_);
	layout_.addWidget(&button_ellipsoid_);
	layout_.addWidget(&button_box_);
	layout_.addWidget(&button_cylinder_);
	layout_.addWidget(&button_cone_);
	layout_.addWidget(&button_paraboloid_);
	layout_.addWidget(&button_hyperboloid_);
	layout_.addWidget(&button_hyperbolic_paraboloid_);

	setLayout(&layout_);
}

void NewNodeListWidget::OnAddMulChain()
{
	CSGTree::MulChain mul_chain
	{ {
		CSGTree::Ellipsoid{ { 0.0f, 0.0f, +0.25f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
		CSGTree::Ellipsoid{ { 0.0f, 0.0f, -0.25f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
	} };
	node_add_callback_(std::move(mul_chain));
}

void NewNodeListWidget::OnAddAddChain()
{
	CSGTree::AddChain add_chain
	{ {
		CSGTree::Ellipsoid{ { 0.0f, 0.0f, +0.25f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
		CSGTree::Ellipsoid{ { 0.0f, 0.0f, -0.25f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
	} };
	node_add_callback_(std::move(add_chain));
}

void NewNodeListWidget::OnAddSubChain()
{
	CSGTree::SubChain sub_chain
	{ {
		CSGTree::Ellipsoid{ { 0.0f, 0.0f, +0.25f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
		CSGTree::Ellipsoid{ { 0.0f, 0.0f, -0.25f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
	} };
	node_add_callback_(std::move(sub_chain));
}

void NewNodeListWidget::OnAddEllipsoid()
{
	CSGTree::Ellipsoid ellipsoid{};
	ellipsoid.size= m_Vec3(1.0f, 1.0f, 1.0f);
	ellipsoid.normal= m_Vec3(0.0f, 0.0f, 1.0f);
	ellipsoid.binormal= m_Vec3(1.0f, 0.0f, 0.0f);
	node_add_callback_(ellipsoid);
}

void NewNodeListWidget::OnAddBox()
{
	CSGTree::Box box{};
	box.size= m_Vec3(1.0f, 1.0f, 1.0f);
	box.normal= m_Vec3(0.0f, 0.0f, 1.0f);
	box.binormal= m_Vec3(1.0f, 0.0f, 0.0f);
	node_add_callback_(box);
}

void NewNodeListWidget::OnAddCylinder()
{
	CSGTree::Cylinder elliptic_cylinder{};
	elliptic_cylinder.size= m_Vec3(1.0f, 1.0f, 1.0f);
	elliptic_cylinder.normal= m_Vec3(0.0f, 0.0f, 1.0f);
	elliptic_cylinder.binormal= m_Vec3(1.0f, 0.0f, 0.0f);
	node_add_callback_(elliptic_cylinder);
}

void NewNodeListWidget::OnAddCone()
{
	CSGTree::Cone cone{};
	cone.angle= 0.5f;
	cone.height= 1.0f;
	cone.normal= m_Vec3(0.0f, 0.0f, 1.0f);
	cone.binormal= m_Vec3(1.0f, 0.0f, 0.0f);
	node_add_callback_(cone);
}

void NewNodeListWidget::OnAddParaboloid()
{
	CSGTree::Paraboloid paraboloid{};
	paraboloid.size= m_Vec3( 1.0f, 1.0f, 1.0f );
	paraboloid.normal= m_Vec3(0.0f, 0.0f, 1.0f);
	paraboloid.binormal= m_Vec3(1.0f, 0.0f, 0.0f);
	node_add_callback_(paraboloid);
}

void NewNodeListWidget::OnAddHyperboloid()
{
	CSGTree::Hyperboloid hyperboloid{};
	hyperboloid.factor= 1.5f;
	hyperboloid.height= 1.0f;
	hyperboloid.radius= 0.125f;
	hyperboloid.normal= m_Vec3(0.0f, 0.0f, 1.0f);
	hyperboloid.binormal= m_Vec3(1.0f, 0.0f, 0.0f);
	node_add_callback_(hyperboloid);
}

void NewNodeListWidget::OnAddHyperbolicParaboloid()
{
	CSGTree::HyperbolicParaboloid hyperbolic_paraboloid{};
	hyperbolic_paraboloid.height= 1.0f;
	hyperbolic_paraboloid.normal= m_Vec3(0.0f, 0.0f, 1.0f);
	hyperbolic_paraboloid.binormal= m_Vec3(1.0f, 0.0f, 0.0f);
	node_add_callback_(hyperbolic_paraboloid);
}

} // namespace SZV
