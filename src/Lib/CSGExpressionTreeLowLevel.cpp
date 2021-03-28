#include "CSGExpressionTreeLowLevel.hpp"
#include "Mat.hpp"

namespace SZV
{

namespace
{

GPUSurface TransformSurface_impl(
	const GPUSurface& s,
	const m_Vec3& shift,
	const m_Vec3& x_vec, const m_Vec3& y_vec, const m_Vec3& z_vec)
{
	GPUSurface r{};

	r.xx=
		s.xx * (x_vec.x * x_vec.x) +
		s.yy * (y_vec.x * y_vec.x) +
		s.zz * (z_vec.x * z_vec.x) +
		s.xy * x_vec.x * y_vec.x +
		s.xz * x_vec.x * z_vec.x +
		s.yz * y_vec.x * z_vec.x;
	r.yy=
		s.xx * (x_vec.y * x_vec.y) +
		s.yy * (y_vec.y * y_vec.y) +
		s.zz * (z_vec.y * z_vec.y) +
		s.xy * x_vec.y * y_vec.y +
		s.xz * x_vec.y * z_vec.y +
		s.yz * y_vec.y * z_vec.y;
	r.zz=
		s.xx * (x_vec.z * x_vec.z) +
		s.yy * (y_vec.z * y_vec.z) +
		s.zz * (z_vec.z * z_vec.z) +
		s.xy * x_vec.z * y_vec.z +
		s.xz * x_vec.z * z_vec.z +
		s.yz * y_vec.z * z_vec.z;

	r.xy=
		2.0f * (s.xx * x_vec.x * x_vec.y + s.yy * y_vec.x * y_vec.y + s.zz * z_vec.x * z_vec.y) +
		s.xy * (x_vec.x * y_vec.y + y_vec.x * x_vec.y) +
		s.xz * (x_vec.x * z_vec.y + z_vec.x * x_vec.y) +
		s.yz * (y_vec.x * z_vec.y + z_vec.x * y_vec.y);
	r.xz=
		2.0f * (s.xx * x_vec.x * x_vec.z + s.yy * y_vec.x * y_vec.z + s.zz * z_vec.x * z_vec.z) +
		s.xy * (x_vec.x * y_vec.z + y_vec.x * x_vec.z) +
		s.xz * (x_vec.x * z_vec.z + z_vec.x * x_vec.z) +
		s.yz * (y_vec.x * z_vec.z + z_vec.x * y_vec.z);
	r.yz=
		2.0f * (s.xx * x_vec.y * x_vec.z + s.yy * y_vec.y * y_vec.z + s.zz * z_vec.y * z_vec.z) +
		s.xy * (x_vec.y * y_vec.z + y_vec.y * x_vec.z) +
		s.xz * (x_vec.y * z_vec.z + z_vec.y * x_vec.z) +
		s.yz * (y_vec.y * z_vec.z + z_vec.y * y_vec.z);

	r.x=
		2.0f * (s.xx * x_vec.x * shift.x + s.yy * y_vec.x * shift.y + s.zz * z_vec.x * shift.z) +
		s.xy * (x_vec.x * shift.y + y_vec.x * shift.x) +
		s.xz * (x_vec.x * shift.z + z_vec.x * shift.x) +
		s.yz * (y_vec.x * shift.z + z_vec.x * shift.y) +
		(s.x * x_vec.x + s.y * y_vec.x + s.z * z_vec.x);
	r.y=
		2.0f * (s.xx * x_vec.y * shift.x + s.yy * y_vec.y * shift.y + s.zz * z_vec.y * shift.z) +
		s.xy * (x_vec.y * shift.y + y_vec.y * shift.x) +
		s.xz * (x_vec.y * shift.z + z_vec.y * shift.x) +
		s.yz * (y_vec.y * shift.z + z_vec.y * shift.y) +
		(s.x * x_vec.y + s.y * y_vec.y + s.z * z_vec.y);
	r.z=
		2.0f * (s.xx * x_vec.z * shift.x + s.yy * y_vec.z * shift.y + s.zz * z_vec.z * shift.z) +
		s.xy * (x_vec.z * shift.y + y_vec.z * shift.x) +
		s.xz * (x_vec.z * shift.z + z_vec.z * shift.x) +
		s.yz * (y_vec.z * shift.z + z_vec.z * shift.y) +
		(s.x * x_vec.z + s.y * y_vec.z + s.z * z_vec.z);

	r.k=
		s.xx * shift.x * shift.x + s.yy * shift.y * shift.y + s.zz * shift.z * shift.z +
		s.xy * shift.x * shift.y +
		s.xz * shift.x * shift.z +
		s.yz * shift.y * shift.z +
		s.x * shift.x + s.y * shift.y + s.z * shift.z +
		s.k;

	return r;
}

// Warning!
// normal and binormal vectors must be perpendicular and must have identity length!
GPUSurface TransformSurface(const GPUSurface& s, const m_Vec3& center, const m_Vec3& normal, const m_Vec3& binormal)
{
	const m_Vec3 tangent= mVec3Cross(normal, binormal);

	return
		TransformSurface_impl(
			s,
			-m_Vec3(mVec3Dot(center, binormal), mVec3Dot(center, tangent), mVec3Dot(center, normal)),
			binormal,
			tangent,
			normal);
}

// Warning!
// normal and binormal vectors must be perpendicular and must have identity length!
BoundingBox TransformBoundingBox(const BoundingBox& bb, const m_Vec3& center, const m_Vec3& normal, const m_Vec3& binormal)
{
	m_Mat4 rotate, translate, full_transform;

	const m_Vec3 tangent= mVec3Cross(normal, binormal);

	rotate.MakeIdentity();
	rotate.value[ 0]= binormal.x;
	rotate.value[ 1]= binormal.y;
	rotate.value[ 2]= binormal.z;
	rotate.value[ 4]= tangent.x;
	rotate.value[ 5]= tangent.y;
	rotate.value[ 6]= tangent.z;
	rotate.value[ 8]= normal.x;
	rotate.value[ 9]= normal.y;
	rotate.value[10]= normal.z;

	translate.Translate(center);

	full_transform= rotate * translate;

	const m_Vec3 bb_vertices[8]=
	{
		m_Vec3(bb.min.x, bb.min.y, bb.min.z),
		m_Vec3(bb.min.x, bb.min.y, bb.max.z),
		m_Vec3(bb.min.x, bb.max.y, bb.min.z),
		m_Vec3(bb.min.x, bb.max.y, bb.max.z),
		m_Vec3(bb.max.x, bb.min.y, bb.min.z),
		m_Vec3(bb.max.x, bb.min.y, bb.max.z),
		m_Vec3(bb.max.x, bb.max.y, bb.min.z),
		m_Vec3(bb.max.x, bb.max.y, bb.max.z),
	};

	const float inf= 1.0e24f;
	BoundingBox res{ { +inf, +inf, +inf }, { -inf, -inf, -inf } };
	for (const m_Vec3& v : bb_vertices )
	{
		const m_Vec3 v_transformed= v * full_transform;
		res.min.x= std::min( res.min.x, v_transformed.x );
		res.min.y= std::min( res.min.y, v_transformed.y );
		res.min.z= std::min( res.min.z, v_transformed.z );
		res.max.x= std::max( res.max.x, v_transformed.x );
		res.max.y= std::max( res.max.y, v_transformed.y );
		res.max.z= std::max( res.max.z, v_transformed.z );
	}

	return res;
}

TreeElementsLowLevel::TreeElement BuildLowLevelTree_r(GPUSurfacesVector& out_surfaces, const CSGTree::CSGTreeNode& node);

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::MulChain& node)
{
	if(node.elements.empty())
		return TreeElementsLowLevel::OneLeaf{};
	else if(node.elements.size() == 1u)
		return BuildLowLevelTree_r(out_surfaces, node.elements.front());

	TreeElementsLowLevel::Mul mul;
	mul.l= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[0]));
	mul.r= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[1]));

	for (size_t i= 2u; i < node.elements.size(); ++i)
	{
		TreeElementsLowLevel::Mul mul_element;
		mul_element.l= std::make_unique<TreeElementsLowLevel::TreeElement>(std::move(mul));
		mul_element.r= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[i]));
		mul= std::move(mul_element);
	}

	return std::move(mul);
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::AddChain& node)
{
	if(node.elements.empty())
		return TreeElementsLowLevel::OneLeaf{};
	else if(node.elements.size() == 1u)
		return BuildLowLevelTree_r(out_surfaces, node.elements.front());

	TreeElementsLowLevel::Add add;
	add.l= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[0]));
	add.r= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[1]));

	for (size_t i= 2u; i < node.elements.size(); ++i)
	{
		TreeElementsLowLevel::Add add_element;
		add_element.l= std::make_unique<TreeElementsLowLevel::TreeElement>(std::move(add));
		add_element.r= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[i]));
		add= std::move(add_element);
	}

	return std::move(add);
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::SubChain& node)
{
	if(node.elements.empty())
		return TreeElementsLowLevel::OneLeaf{};
	else if(node.elements.size() == 1u)
		return BuildLowLevelTree_r(out_surfaces, node.elements.front());

	TreeElementsLowLevel::Sub sub;
	sub.l= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[0]));
	sub.r= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[1]));

	for (size_t i= 2u; i < node.elements.size(); ++i)
	{
		TreeElementsLowLevel::Sub sub_element;
		sub_element.l= std::make_unique<TreeElementsLowLevel::TreeElement>(std::move(sub));
		sub_element.r= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[i]));
		sub= std::move(sub_element);
	}

	return std::move(sub);
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::Ellipsoid& node)
{
	GPUSurface surface{};
	surface.xx= 4.0f / (node.size.x * node.size.x);
	surface.yy= 4.0f / (node.size.y * node.size.y);
	surface.zz= 4.0f / (node.size.z * node.size.z);
	surface.k= -1.0f;
	surface= TransformSurface(surface, node.center, node.normal, node.binormal);

	const size_t surface_index= out_surfaces.size();
	out_surfaces.push_back(surface);

	const BoundingBox bb{ -node.size * 0.5f, node.size * 0.5f };

	TreeElementsLowLevel::Leaf leaf;
	leaf.surface_index= surface_index;
	leaf.bb= TransformBoundingBox(bb, node.center, node.normal, node.binormal);
	return leaf;
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::Cube& node)
{
	// Represent three pairs of parallel planes of cube using three quadratic surfaces.
	const m_Vec3 normal(0.0f, 0.0f, 1.0f);
	const m_Vec3 binormal(1.0, 0.0f, 0.0f);
	const size_t surface_index= out_surfaces.size();

	{
		GPUSurface surface{};
		surface.xx= 1.0f;
		surface.k= -0.25f * node.size.x * node.size.x;
		surface= TransformSurface(surface, node.center, normal, binormal);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.yy= 1.0f;
		surface.k= -0.25f * node.size.y * node.size.y;
		surface= TransformSurface(surface, node.center, normal, binormal);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.zz= 1.0f;
		surface.k= -0.25f * node.size.z * node.size.z;
		surface= TransformSurface(surface, node.center, normal, binormal);
		out_surfaces.push_back(surface);
	}

	const BoundingBox bb
	{
		{ -node.size.x * 0.5f, -node.size.y * 0.5f, -node.size.z * 0.5f },
		{ +node.size.x * 0.5f, +node.size.y * 0.5f, +node.size.z * 0.5f },
	};
	const BoundingBox bb_transformed= TransformBoundingBox(bb, node.center, normal, binormal);

	TreeElementsLowLevel::Leaf leafs[3];
	for (size_t i= 0u; i < 3u; ++i)
	{
		leafs[i].surface_index= surface_index + i;
		leafs[i].bb= bb_transformed;
	}

	return TreeElementsLowLevel::Mul
	{
		std::make_unique<TreeElementsLowLevel::TreeElement>(
			TreeElementsLowLevel::Mul
			{
				std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[0]),
				std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[1]),
			}),
		std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[2]),
	};
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::Cylinder& node)
{
	const size_t surface_index= out_surfaces.size();

	{
		GPUSurface surface{};
		surface.xx= 4.0f / (node.size.x * node.size.x);
		surface.yy= 4.0f / (node.size.y * node.size.y);
		surface.k= -1.0f;
		surface= TransformSurface(surface, node.center, node.normal, node.binormal);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.zz= 1.0f;
		surface.k= -0.25f * node.size.z * node.size.z;
		surface= TransformSurface(surface, node.center, node.normal, node.binormal);
		out_surfaces.push_back(surface);
	}

	const BoundingBox bb{ -node.size * 0.5f, node.size * 0.5f };
	const BoundingBox bb_transformed= TransformBoundingBox(bb, node.center, node.normal, node.binormal);

	TreeElementsLowLevel::Leaf leafs[2];
	for (size_t i= 0u; i < 2u; ++i)
	{
		leafs[i].surface_index= surface_index + i;
		leafs[i].bb= bb_transformed;
	}

	return TreeElementsLowLevel::Mul
	{
		std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[0]),
		std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[1]),
	};
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::Cone& node)
{
	const float k= std::tan(node.angle * 0.5f);

	const size_t surface_index= out_surfaces.size();
	{
		GPUSurface surface{};
		surface.xx= surface.yy= 1.0f;
		surface.zz= -k * k;
		surface= TransformSurface(surface, node.center, node.normal, node.binormal);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.zz= 1.0f;
		surface.z= -node.height;
		surface.k= 0.0f;
		surface= TransformSurface(surface, node.center, node.normal, node.binormal);
		out_surfaces.push_back(surface);
	}

	const float top_radius= k * node.height;
	const BoundingBox bb
	{
		{ -top_radius, -top_radius, 0.0f },
		{ +top_radius, +top_radius, node.height },
	};
	const BoundingBox bb_transformed= TransformBoundingBox(bb, node.center, node.normal, node.binormal);

	TreeElementsLowLevel::Leaf leafs[2];
	for (size_t i= 0u; i < 2u; ++i)
	{
		leafs[i].surface_index= surface_index + i;
		leafs[i].bb= bb_transformed;
	}

	return TreeElementsLowLevel::Mul
	{
		std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[0]),
		std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[1]),
	};
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::Paraboloid& node)
{
	const size_t surface_index= out_surfaces.size();

	{
		GPUSurface surface{};
		surface.xx= surface.yy= 1.0f;
		surface.z= -node.factor;
		surface= TransformSurface(surface, node.center, node.normal, node.binormal);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.zz= 1.0f;
		surface.z= -node.height;
		surface.k= 0.0f;
		surface= TransformSurface(surface, node.center, node.normal, node.binormal);
		out_surfaces.push_back(surface);
	}

	const float top_radius= std::sqrt( node.factor * node.height );
	const BoundingBox bb
	{
		{ -top_radius, -top_radius, 0.0f },
		{ +top_radius, +top_radius, node.height },
	};
	const BoundingBox bb_transformed= TransformBoundingBox(bb, node.center, node.normal, node.binormal);

	TreeElementsLowLevel::Leaf leafs[2];
	for (size_t i= 0u; i < 2u; ++i)
	{
		leafs[i].surface_index= surface_index + i;
		leafs[i].bb= bb_transformed;
	}

	return TreeElementsLowLevel::Mul
	{
		std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[0]),
		std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[1]),
	};
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::Hyperboloid& node)
{
	const size_t surface_index= out_surfaces.size();

	{
		GPUSurface surface{};
		surface.xx= surface.yy= 1.0f;
		surface.zz= -node.factor * node.factor;
		surface.k= -std::abs(node.radius) * node.radius;
		surface= TransformSurface(surface, node.center, node.normal, node.binormal);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.zz= 1.0f;
		surface.k= -0.25f * node.height * node.height;
		surface= TransformSurface(surface, node.center, node.normal, node.binormal);
		out_surfaces.push_back(surface);
	}

	const float max_radius= std::sqrt( std::max( 0.0f, std::abs(node.radius) * node.radius + 0.25f * node.height * node.height * node.factor * node.factor ) );
	const BoundingBox bb
	{
		{ -max_radius, -max_radius, -0.5f * node.height },
		{ +max_radius, +max_radius, +0.5f * node.height },
	};
	const BoundingBox bb_transformed= TransformBoundingBox(bb, node.center, node.normal, node.binormal);

	TreeElementsLowLevel::Leaf leafs[2];
	for (size_t i= 0u; i < 2u; ++i)
	{
		leafs[i].surface_index= surface_index + i;
		leafs[i].bb= bb_transformed;
	}

	return TreeElementsLowLevel::Mul
	{
		std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[0]),
		std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[1]),
	};
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::HyperbolicParaboloid& node)
{
	const size_t surface_index= out_surfaces.size();

	{ // Hyperbolic paraboloid itself.
		GPUSurface surface{};
		surface.xx= +1.0f;
		surface.yy= -1.0f;
		surface.z= 1.0f;
		surface= TransformSurface(surface, node.center, node.normal, node.binormal);
		out_surfaces.push_back(surface);
	}
	{ // Pair of bounding planes.
		GPUSurface surface{};
		surface.yy= 1.0f;
		surface.k= -0.5f * node.height;
		surface= TransformSurface(surface, node.center, node.normal, node.binormal);
		out_surfaces.push_back(surface);
	}
	{ // Single bounding plane.
		GPUSurface surface{};
		surface.z= -1.0f;
		surface.k= -0.5f * node.height;
		surface= TransformSurface(surface, node.center, node.normal, node.binormal);
		out_surfaces.push_back(surface);
	}

	const float half_size_x= std::sqrt(node.height);
	const float half_size_y= half_size_x * std::sqrt(0.5f);
	const BoundingBox bb
	{
		{ -half_size_x, -half_size_y, -0.5f * node.height },
		{ +half_size_x, +half_size_y, +0.5f * node.height },
	};
	const BoundingBox bb_transformed= TransformBoundingBox(bb, node.center, node.normal, node.binormal);

	TreeElementsLowLevel::Leaf leafs[3];
	for (size_t i= 0u; i < 3u; ++i)
	{
		leafs[i].surface_index= surface_index + i;
		leafs[i].bb= bb_transformed;
	}

	return TreeElementsLowLevel::Mul
	{
		std::make_unique<TreeElementsLowLevel::TreeElement>(
			TreeElementsLowLevel::Mul
			{
				std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[0]),
				std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[1]),
			}),
		std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[2]),
	};
}

TreeElementsLowLevel::TreeElement BuildLowLevelTree_r(GPUSurfacesVector& out_surfaces, const CSGTree::CSGTreeNode& node)
{
	return std::visit(
		[&](const auto& el)
		{
			return BuildLowLevelTreeNode_impl(out_surfaces, el);
		},
		node);
}

} // namespace

TreeElementsLowLevel::TreeElement BuildLowLevelTree(GPUSurfacesVector& out_surfaces, const CSGTree::CSGTreeNode& root)
{
	return BuildLowLevelTree_r(out_surfaces, root);
}

} // namespace SZV
