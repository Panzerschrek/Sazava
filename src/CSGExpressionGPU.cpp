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
	Cube= 102,
	Cylinder= 103,
};

namespace ExpressionElements
{

struct Sphere
{
	float center[3];
	float radius;
};

struct Cube
{
	float center[3];
	float half_size[3];
};

struct Cylinder
{
	float center[3];
	float normal[3];
	float radius;
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
	ExpressionElements::Sphere sphere{};
	sphere.center[0]= node.center[0];
	sphere.center[1]= node.center[1];
	sphere.center[2]= node.center[2];
	sphere.radius= node.radius;

	AppendExpressionComponent(out_expression, ExpressionElementType::Sphere, sphere);
}

void ConvertCSGTreeNode_impl(CSGExpressionGPU& out_expression, const CSGTree::Cube& node)
{
	ExpressionElements::Cube cube{};
	cube.center[0]= node.center[0];
	cube.center[1]= node.center[1];
	cube.center[2]= node.center[2];
	cube.half_size[0]= node.size[0] * 0.5f;
	cube.half_size[1]= node.size[1] * 0.5f;
	cube.half_size[2]= node.size[2] * 0.5f;

	AppendExpressionComponent(out_expression, ExpressionElementType::Cube, cube);
}

void ConvertCSGTreeNode_impl(CSGExpressionGPU& out_expression, const CSGTree::Cylinder& node)
{
	ExpressionElements::Cylinder cylinder{};
	cylinder.center[0]= node.center[0];
	cylinder.center[1]= node.center[1];
	cylinder.center[2]= node.center[2];
	cylinder.normal[0]= node.normal[0];
	cylinder.normal[1]= node.normal[1];
	cylinder.normal[2]= node.normal[2];
	cylinder.radius= node.radius;

	AppendExpressionComponent(out_expression, ExpressionElementType::Cylinder, cylinder);
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
