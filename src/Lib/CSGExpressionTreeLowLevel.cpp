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

using BasisVecs= std::array<m_Vec3, 3>;

BasisVecs GetTransformedBasis(const m_Vec3& angles)
{
	const float deg2rad= 3.1415926535f / 180.0f;

	m_Mat4 rot_x, rot_y, rot_z, rot;
	rot_x.RotateX(angles.x * deg2rad);
	rot_y.RotateY(angles.y * deg2rad);
	rot_z.RotateZ(angles.z * deg2rad);
	rot= rot_x * rot_y * rot_z;

	return
	{
		m_Vec3( rot.value[0], rot.value[4], rot.value[ 8] ),
		m_Vec3( rot.value[1], rot.value[5], rot.value[ 9] ),
		m_Vec3( rot.value[2], rot.value[6], rot.value[10] ),
	};
}

// Warning!
// Basis vectors must be perpendicular and must have identity length!
GPUSurface TransformSurface(const GPUSurface& s, const m_Vec3& center, const BasisVecs& basis_vecs)
{
	return
		TransformSurface_impl(
			s,
			-m_Vec3(mVec3Dot(center, basis_vecs[0]), mVec3Dot(center, basis_vecs[1]), mVec3Dot(center, basis_vecs[2])),
			basis_vecs[0],
			basis_vecs[1],
			basis_vecs[2]);
}

// Warning!
// normal and binormal vectors must be perpendicular and must have identity length!
BoundingBox TransformBoundingBox(const BoundingBox& bb, const m_Vec3& center, const BasisVecs& basis_vecs)
{
	m_Mat4 rotate, translate, full_transform;

	rotate.MakeIdentity();
	rotate.value[ 0]= basis_vecs[0].x;
	rotate.value[ 1]= basis_vecs[0].y;
	rotate.value[ 2]= basis_vecs[0].z;
	rotate.value[ 4]= basis_vecs[1].x;
	rotate.value[ 5]= basis_vecs[1].y;
	rotate.value[ 6]= basis_vecs[1].z;
	rotate.value[ 8]= basis_vecs[2].x;
	rotate.value[ 9]= basis_vecs[2].y;
	rotate.value[10]= basis_vecs[2].z;

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
	const auto basis= GetTransformedBasis(node.angles_deg);

	GPUSurface surface{};
	surface.xx= 4.0f / (node.size.x * node.size.x);
	surface.yy= 4.0f / (node.size.y * node.size.y);
	surface.zz= 4.0f / (node.size.z * node.size.z);
	surface.k= -1.0f;
	surface= TransformSurface(surface, node.center, basis);

	const size_t surface_index= out_surfaces.size();
	out_surfaces.push_back(surface);

	const BoundingBox bb{ -node.size * 0.5f, node.size * 0.5f };

	TreeElementsLowLevel::Leaf leaf;
	leaf.surface_index= surface_index;
	leaf.bb= TransformBoundingBox(bb, node.center, basis);
	return leaf;
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::Box& node)
{
	const auto basis= GetTransformedBasis(node.angles_deg);

	// Represent three pairs of parallel planes of box using three quadratic surfaces.
	const size_t surface_index= out_surfaces.size();

	{
		GPUSurface surface{};
		surface.xx= 1.0f;
		surface.k= -0.25f * node.size.x * node.size.x;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.yy= 1.0f;
		surface.k= -0.25f * node.size.y * node.size.y;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.zz= 1.0f;
		surface.k= -0.25f * node.size.z * node.size.z;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}

	const BoundingBox bb
	{
		{ -node.size.x * 0.5f, -node.size.y * 0.5f, -node.size.z * 0.5f },
		{ +node.size.x * 0.5f, +node.size.y * 0.5f, +node.size.z * 0.5f },
	};
	const BoundingBox bb_transformed= TransformBoundingBox(bb, node.center, basis);

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
	const auto basis= GetTransformedBasis(node.angles_deg);

	const size_t surface_index= out_surfaces.size();

	{
		GPUSurface surface{};
		surface.xx= 4.0f / (node.size.x * node.size.x);
		surface.yy= 4.0f / (node.size.y * node.size.y);
		surface.k= -1.0f;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.zz= 1.0f;
		surface.k= -0.25f * node.size.z * node.size.z;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}

	const BoundingBox bb{ -node.size * 0.5f, node.size * 0.5f };
	const BoundingBox bb_transformed= TransformBoundingBox(bb, node.center, basis);

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
	const auto basis= GetTransformedBasis(node.angles_deg);

	const size_t surface_index= out_surfaces.size();
	{
		GPUSurface surface{};
		surface.xx= node.size.z * node.size.z * 4.0f / (node.size.x * node.size.x);
		surface.yy= node.size.z * node.size.z * 4.0f / (node.size.y * node.size.y);
		surface.zz= -1.0f;
		surface.z= -node.size.z;
		surface.k= -0.25f * node.size.z * node.size.z;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.zz= 1.0f;
		surface.k= -node.size.z * node.size.z * 0.25f;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}

	const BoundingBox bb{ -node.size * 0.5f, node.size * 0.5f };
	const BoundingBox bb_transformed= TransformBoundingBox(bb, node.center, basis);

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
	const auto basis= GetTransformedBasis(node.angles_deg);

	const size_t surface_index= out_surfaces.size();

	{
		GPUSurface surface{};
		surface.xx= node.size.z * 4.0f / (node.size.x * node.size.x);
		surface.yy= node.size.z * 4.0f / (node.size.y * node.size.y);
		surface.z= -1.0f;
		surface.k= -0.5f * node.size.z;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.z= 1.0f;
		surface.k= -node.size.z * 0.5f;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}

	const BoundingBox bb{ -node.size * 0.5f, node.size * 0.5f };
	const BoundingBox bb_transformed= TransformBoundingBox(bb, node.center, basis);

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
	const auto basis= GetTransformedBasis(node.angles_deg);

	const size_t surface_index= out_surfaces.size();

	{
		const float k= 0.25f * std::abs(node.focus_distance) * node.focus_distance;
		GPUSurface surface{};
		surface.xx= (node.size.z * node.size.z - k * 4.0f) / (node.size.x * node.size.x);
		surface.yy= (node.size.z * node.size.z - k * 4.0f) / (node.size.y * node.size.y);
		surface.zz= -1.0f;
		surface.k= k;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.zz= 1.0f;
		surface.k= -0.25f * node.size.z * node.size.z;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}

	const BoundingBox bb{ -node.size * 0.5f, node.size * 0.5f };
	const BoundingBox bb_transformed= TransformBoundingBox(bb, node.center, basis);

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
	const auto basis= GetTransformedBasis(node.angles_deg);

	const size_t surface_index= out_surfaces.size();

	{ // Hyperbolic paraboloid itself.
		GPUSurface surface{};
		surface.xx= +1.0f;
		surface.yy= -1.0f;
		surface.z= 1.0f;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}
	{ // Pair of bounding planes.
		GPUSurface surface{};
		surface.yy= 1.0f;
		surface.k= -0.5f * node.height;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}
	{ // Single bounding plane.
		GPUSurface surface{};
		surface.z= -1.0f;
		surface.k= -0.5f * node.height;
		surface= TransformSurface(surface, node.center, basis);
		out_surfaces.push_back(surface);
	}

	const float half_size_x= std::sqrt(node.height);
	const float half_size_y= half_size_x * std::sqrt(0.5f);
	const BoundingBox bb
	{
		{ -half_size_x, -half_size_y, -0.5f * node.height },
		{ +half_size_x, +half_size_y, +0.5f * node.height },
	};
	const BoundingBox bb_transformed= TransformBoundingBox(bb, node.center, basis);

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
