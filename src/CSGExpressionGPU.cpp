//
// ATTENTION!
//

// This file contains representation of structs for shader.
// If this code changed, corresponding shader must be changed too!

#include <cassert>
#include "CSGExpressionGPU.hpp"

namespace SZV
{

namespace
{

enum class ExpressionElementType
{
	// Operations. Have no parameters.
	End= 0,
	Mul= 1,
	Add= 2,
	Sub= 3,

	// Objects. Have parameters (specific count for each element).
	Sphere= 101,
	Plane= 102,
	Cylinder= 103,
	Cone= 104,
	Paraboloid= 105,
};

namespace ExpressionElements
{

struct Sphere
{
	float center[3];
	float square_radius;
};

struct Plane
{
	float center[3];
	float normal[3]; // Actually, not a normal, but some vector with non-zero length
	float binormal[3];
};

struct Cylinder
{
	float center[3];
	float normal[3];
	float binormal[3];
	float square_radius;
};

struct Cone
{
	float center[3];
	// Vectors must be normalized and perpendicular!
	float normal[3]; // main axis of the cone
	float binormal[3];
	float square_tangenrt;
};

struct Paraboloid
{
	float center[3];
	// Vectors must be normalized and perpendicular!
	float normal[3]; // main axis of the cone
	float binormal[3];
	float factor;
};

} // namespace ExpressionElements

void AppendExpressionComponent(CSGExpressionGPU& out_expression, const ExpressionElementType el)
{
	out_expression.push_back( float(el) );
}

template<typename T>
void AppendExpressionComponent(CSGExpressionGPU& out_expression, const ExpressionElementType el, const T& data)
{
	AppendExpressionComponent(out_expression, el);

	static_assert( sizeof(T) % sizeof(float) == 0, "Invalid alignment" );
	out_expression.insert(
		out_expression.end(),
		reinterpret_cast<const float*>(&data),
		reinterpret_cast<const float*>(&data) + sizeof(T) / sizeof(float));
}

void ConvertCSGTreeNode(CSGExpressionGPU& out_expression, const CSGTree::CSGTreeNode& node);

void ConvertCSGTreeNode_impl(CSGExpressionGPU& out_expression, const CSGTree::MulChain& node)
{
	assert( node.elements.size() >= 1u );
	ConvertCSGTreeNode(out_expression, node.elements.front());
	for (size_t i= 1; i < node.elements.size(); ++i)
	{
		ConvertCSGTreeNode(out_expression, node.elements[i]);
		AppendExpressionComponent(out_expression, ExpressionElementType::Mul);
	}
}

void ConvertCSGTreeNode_impl(CSGExpressionGPU& out_expression, const CSGTree::AddChain& node)
{
	assert( node.elements.size() >= 1u );
	ConvertCSGTreeNode(out_expression, node.elements.front());
	for (size_t i= 1; i < node.elements.size(); ++i)
	{
		ConvertCSGTreeNode(out_expression, node.elements[i]);
		AppendExpressionComponent(out_expression, ExpressionElementType::Add);
	}
}

void ConvertCSGTreeNode_impl(CSGExpressionGPU& out_expression, const CSGTree::SubChain& node)
{
	assert( node.elements.size() >= 1u );
	ConvertCSGTreeNode(out_expression, node.elements.front());
	for (size_t i= 1; i < node.elements.size(); ++i)
	{
		ConvertCSGTreeNode(out_expression, node.elements[i]);
		AppendExpressionComponent(out_expression, ExpressionElementType::Sub);
	}
}

void ConvertCSGTreeNode_impl(CSGExpressionGPU& out_expression, const CSGTree::Sphere& node)
{
	const ExpressionElements::Sphere sphere{ { node.center.x, node.center.y, node.center.z }, node.radius * node.radius };
	AppendExpressionComponent(out_expression, ExpressionElementType::Sphere, sphere);
}

void ConvertCSGTreeNode_impl(CSGExpressionGPU& out_expression, const CSGTree::Cube& node)
{
	const ExpressionElements::Plane plane_x_plus
	{
		{ node.center.x + node.size.x * 0.5f, node.center.y, node.center.z },
		{ +1.0f, 0.0f, 0.0f },
		{ 0.0f, +1.0f, 0.0f },
	};
	const ExpressionElements::Plane plane_x_minus
	{
		{ node.center.x - node.size.x * 0.5f, node.center.y, node.center.z },
		{ -1.0f, 0.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f },
	};
	const ExpressionElements::Plane plane_y_plus
	{
		{ node.center.x, node.center.y + node.size.y * 0.5f, node.center.z },
		{ 0.0f, +1.0f, 0.0f },
		{ +1.0f, 0.0f, 0.0f },
	};
	const ExpressionElements::Plane plane_y_minus
	{
		{ node.center.x, node.center.y - node.size.y * 0.5f, node.center.z },
		{ 0.0f, -1.0f, 0.0f },
		{ -1.0f, 0.0f, 0.0f },
	};
	const ExpressionElements::Plane plane_z_plus
	{
		{ node.center.x, node.center.y, node.center.z + node.size.z * 0.5f },
		{ 0.0f, 0.0f, +1.0f },
		{ +1.0f, 0.0f, 0.0f },
	};
	const ExpressionElements::Plane plane_z_minus
	{
		{ node.center.x, node.center.y, node.center.z - node.size.z * 0.5f },
		{ 0.0f, 0.0f, -1.0f },
		{ -1.0f, 0.0f, 0.0f },
	};

	AppendExpressionComponent(out_expression, ExpressionElementType::Plane, plane_x_plus );
	AppendExpressionComponent(out_expression, ExpressionElementType::Plane, plane_x_minus);
	AppendExpressionComponent(out_expression, ExpressionElementType::Mul);
	AppendExpressionComponent(out_expression, ExpressionElementType::Plane, plane_y_plus );
	AppendExpressionComponent(out_expression, ExpressionElementType::Mul);
	AppendExpressionComponent(out_expression, ExpressionElementType::Plane, plane_y_minus);
	AppendExpressionComponent(out_expression, ExpressionElementType::Mul);
	AppendExpressionComponent(out_expression, ExpressionElementType::Plane, plane_z_plus );
	AppendExpressionComponent(out_expression, ExpressionElementType::Mul);
	AppendExpressionComponent(out_expression, ExpressionElementType::Plane, plane_z_minus);
	AppendExpressionComponent(out_expression, ExpressionElementType::Mul);
}

void ConvertCSGTreeNode_impl(CSGExpressionGPU& out_expression, const CSGTree::Cylinder& node)
{
	const ExpressionElements::Cylinder cylinder
	{
		{ node.center.x, node.center.y, node.center.z },
		{ node.normal.x, node.normal.y, node.normal.z },
		{ node.binormal.x, node.binormal.y, node.binormal.z },
		node.radius * node.radius,
	};

	AppendExpressionComponent(out_expression, ExpressionElementType::Cylinder, cylinder);

	const m_Vec3 top_plane_point= node.center + node.normal * (node.height * 0.5f);
	const ExpressionElements::Plane top_plane
	{
		{ top_plane_point.x, top_plane_point.y, top_plane_point.z },
		{ node.normal.x, node.normal.y, node.normal.z },
		{ node.binormal.x, node.binormal.y, node.normal.z },
	};

	AppendExpressionComponent(out_expression, ExpressionElementType::Plane, top_plane);
	AppendExpressionComponent(out_expression, ExpressionElementType::Mul);

	const m_Vec3 bottom_plane_point= node.center - node.normal * (node.height * 0.5f);
	const ExpressionElements::Plane bottom_plane
	{
		{ bottom_plane_point.x, bottom_plane_point.y, bottom_plane_point.z },
		{ -node.normal.x, -node.normal.y, -node.normal.z },
		{ node.binormal.x, node.binormal.y, node.normal.z },
	};

	AppendExpressionComponent(out_expression, ExpressionElementType::Plane, bottom_plane);
	AppendExpressionComponent(out_expression, ExpressionElementType::Mul);
}

void ConvertCSGTreeNode_impl(CSGExpressionGPU& out_expression, const CSGTree::Cone& node)
{
	const float tangent= std::tan(node.angle * 0.5f);
	const ExpressionElements::Cone cone
	{
		{ node.center.x, node.center.y, node.center.z },
		{ node.normal.x, node.normal.y, node.normal.z },
		{ node.binormal.x, node.binormal.y, node.binormal.z },
		tangent * tangent,
	};

	AppendExpressionComponent(out_expression, ExpressionElementType::Cone, cone);

	const m_Vec3 top_plane_point= node.center + node.normal * node.height;
	const ExpressionElements::Plane top_plane
	{
		{ top_plane_point.x, top_plane_point.y, top_plane_point.z },
		{ node.normal.x, node.normal.y, node.normal.z },
		{ node.binormal.x, node.binormal.y, node.normal.z },
	};

	AppendExpressionComponent(out_expression, ExpressionElementType::Plane, top_plane);
	AppendExpressionComponent(out_expression, ExpressionElementType::Mul);
}

void ConvertCSGTreeNode_impl(CSGExpressionGPU& out_expression, const CSGTree::Paraboloid& node)
{
	const ExpressionElements::Paraboloid paraboloid
	{
		{ node.center.x, node.center.y, node.center.z },
		{ node.normal.x, node.normal.y, node.normal.z },
		{ node.binormal.x, node.binormal.y, node.binormal.z },
		node.factor,
	};

	AppendExpressionComponent(out_expression, ExpressionElementType::Paraboloid, paraboloid);

	const m_Vec3 top_plane_point= node.center + node.normal * node.height;
	const ExpressionElements::Plane top_plane
	{
		{ top_plane_point.x, top_plane_point.y, top_plane_point.z },
		{ node.normal.x, node.normal.y, node.normal.z },
		{ node.binormal.x, node.binormal.y, node.normal.z },
	};

	AppendExpressionComponent(out_expression, ExpressionElementType::Plane, top_plane);
	AppendExpressionComponent(out_expression, ExpressionElementType::Mul);
}

void ConvertCSGTreeNode(CSGExpressionGPU& out_expression, const CSGTree::CSGTreeNode& node)
{
	std::visit(
		[&](const auto& el)
		{
			ConvertCSGTreeNode_impl(out_expression, el);
		},
		node);
}

} // namespace

CSGExpressionGPU ConvertCSGTreeToGPUExpression(const CSGTree::CSGTreeNode& tree)
{
	CSGExpressionGPU res;
	res.push_back(0.0f);
	ConvertCSGTreeNode(res, tree);
	res.front()= float(res.size());
	return res;
}

} // namespace SZV
