#include "CSGDataGPU.hpp"
#include "Assert.hpp"

namespace SZV
{

namespace
{

// If this changed, surface shader must be chaged too!
enum class GPUCSGExpressionCodes : CSGExpressionGPUBufferType
{
	Mul= 100,
	Add= 101,
	Sub= 102,

	Leaf= 200,
	OneLeaf= 300,
	ZeroLeaf= 301,
};

enum class CSGExpressionBuildResult
{
	Variable,
	AlwaysZero,
	AlwaysOne,
};

using NodesStack= std::vector<const TreeElementsLowLevel::TreeElement*>;

void AddCube(VerticesVector& out_vertices, IndicesVector& out_indices, const BoundingBox& bb, const size_t surface_description_offset)
{
	const size_t start_index= out_vertices.size();

	const m_Vec3 cube_vertices[8]=
	{
		{ bb.max.x, bb.max.y, bb.max.z }, { bb.min.x, bb.max.y, bb.max.z },
		{ bb.max.x, bb.min.y, bb.max.z }, { bb.min.x, bb.min.y, bb.max.z },
		{ bb.max.x, bb.max.y, bb.min.z }, { bb.min.x, bb.max.y, bb.min.z },
		{ bb.max.x, bb.min.y, bb.min.z }, { bb.min.x, bb.min.y, bb.min.z },
	};

	for(const m_Vec3& cube_vertex : cube_vertices)
	{
		const SurfaceVertex v{ { cube_vertex.x, cube_vertex.y, cube_vertex.z }, float(surface_description_offset) };
		out_vertices.push_back(v);
	}

	static const IndexType cube_indices[12 * 3]=
	{
		0, 1, 5,  0, 5, 4,
		0, 4, 6,  0, 6, 2,
		4, 5, 7,  4, 7, 6,
		0, 3, 1,  0, 2, 3,
		2, 7, 3,  2, 6, 7,
		1, 3, 7,  1, 7, 5,
	};

	for(const size_t cube_index : cube_indices)
		out_indices.push_back(IndexType(cube_index + start_index));
}

CSGExpressionBuildResult BUILDCSGExpression_r(CSGExpressionGPUBuffer& out_expression, const BoundingBox& target_bb, const TreeElementsLowLevel::TreeElement& node);

CSGExpressionBuildResult BUILDCSGExpressionNode_impl(CSGExpressionGPUBuffer& out_expression, const BoundingBox& target_bb, const TreeElementsLowLevel::Mul& node)
{
	const size_t prev_size= out_expression.size();
	const CSGExpressionBuildResult l_result= BUILDCSGExpression_r(out_expression, target_bb, *node.l);
	const CSGExpressionBuildResult r_result= BUILDCSGExpression_r(out_expression, target_bb, *node.r);

	if (l_result == CSGExpressionBuildResult::Variable && r_result == CSGExpressionBuildResult::Variable)
	{
		out_expression.push_back(CSGExpressionGPUBufferType(GPUCSGExpressionCodes::Mul));
		return CSGExpressionBuildResult::Variable;
	}
	else if (l_result == CSGExpressionBuildResult::AlwaysZero || r_result == CSGExpressionBuildResult::AlwaysZero)
	{
		out_expression.resize(prev_size);
		return CSGExpressionBuildResult::AlwaysZero;
	}
	else if (l_result == CSGExpressionBuildResult::AlwaysOne)
		return r_result;
	else if (r_result == CSGExpressionBuildResult::AlwaysOne)
		return l_result;
	else SZV_ASSERT(false);
	return CSGExpressionBuildResult::Variable;
}

CSGExpressionBuildResult BUILDCSGExpressionNode_impl(CSGExpressionGPUBuffer& out_expression, const BoundingBox& target_bb, const TreeElementsLowLevel::Add& node)
{
	const size_t prev_size= out_expression.size();
	const CSGExpressionBuildResult l_result= BUILDCSGExpression_r(out_expression, target_bb, *node.l);
	const CSGExpressionBuildResult r_result= BUILDCSGExpression_r(out_expression, target_bb, *node.r);
	if (l_result == CSGExpressionBuildResult::Variable && r_result == CSGExpressionBuildResult::Variable)
	{
		out_expression.push_back(CSGExpressionGPUBufferType(GPUCSGExpressionCodes::Add));
		return CSGExpressionBuildResult::Variable;
	}
	else if (l_result == CSGExpressionBuildResult::AlwaysOne || r_result == CSGExpressionBuildResult::AlwaysOne)
	{
		out_expression.resize(prev_size);
		return CSGExpressionBuildResult::AlwaysOne;
	}
	else if (l_result == CSGExpressionBuildResult::AlwaysZero)
		return r_result;
	else if (r_result == CSGExpressionBuildResult::AlwaysZero)
		return l_result;
	else SZV_ASSERT(false);
	return CSGExpressionBuildResult::Variable;
}

CSGExpressionBuildResult BUILDCSGExpressionNode_impl(CSGExpressionGPUBuffer& out_expression, const BoundingBox& target_bb, const TreeElementsLowLevel::Sub& node)
{
	const size_t prev_size= out_expression.size();
	const CSGExpressionBuildResult l_result= BUILDCSGExpression_r(out_expression, target_bb, *node.l);
	const CSGExpressionBuildResult r_result= BUILDCSGExpression_r(out_expression, target_bb, *node.r);
	if (l_result == CSGExpressionBuildResult::Variable && r_result == CSGExpressionBuildResult::Variable)
	{
		out_expression.push_back(CSGExpressionGPUBufferType(GPUCSGExpressionCodes::Sub));
		return CSGExpressionBuildResult::Variable;
	}
	else if(l_result == CSGExpressionBuildResult::AlwaysZero || r_result == CSGExpressionBuildResult::AlwaysOne)
	{
		out_expression.resize(prev_size);
		return CSGExpressionBuildResult::AlwaysZero;
	}
	else if (r_result == CSGExpressionBuildResult::AlwaysZero)
		return l_result;
	else if (l_result == CSGExpressionBuildResult::AlwaysOne)
	{
		out_expression.resize(prev_size);
		out_expression.push_back(CSGExpressionGPUBufferType(GPUCSGExpressionCodes::OneLeaf));
		BUILDCSGExpression_r(out_expression, target_bb, *node.r);
		out_expression.push_back(CSGExpressionGPUBufferType(GPUCSGExpressionCodes::Sub));
	}
	else SZV_ASSERT(false);
	return CSGExpressionBuildResult::Variable;
}

CSGExpressionBuildResult BUILDCSGExpressionNode_impl(CSGExpressionGPUBuffer& out_expression, const BoundingBox& target_bb, const TreeElementsLowLevel::Leaf& node)
{
	if (node.bb.max.x < target_bb.min.x || node.bb.min.x > target_bb.max.x ||
		node.bb.max.y < target_bb.min.y || node.bb.min.y > target_bb.max.y ||
		node.bb.max.z < target_bb.min.z || node.bb.min.z > target_bb.max.z )
		return CSGExpressionBuildResult::AlwaysZero;

	out_expression.push_back(CSGExpressionGPUBufferType(GPUCSGExpressionCodes::Leaf));
	out_expression.push_back(CSGExpressionGPUBufferType(node.surface_index));
	return CSGExpressionBuildResult::Variable;
}

CSGExpressionBuildResult BUILDCSGExpression_r(CSGExpressionGPUBuffer& out_expression, const BoundingBox& target_bb, const TreeElementsLowLevel::TreeElement& node)
{
	return std::visit(
		[&](const auto& el)
		{
			return BUILDCSGExpressionNode_impl(out_expression, target_bb, el);
		},
		node);
}

void BuildSceneMeshNode_r(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	CSGExpressionGPUBuffer& out_expressions,
	NodesStack& nodes_stack,
	const TreeElementsLowLevel::TreeElement& node);

void BuildSceneMeshNode_impl(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	CSGExpressionGPUBuffer& out_expressions,
	NodesStack& nodes_stack,
	const TreeElementsLowLevel::Mul& node)
{
	BuildSceneMeshNode_r(out_vertices, out_indices, out_expressions, nodes_stack, *node.l);
	BuildSceneMeshNode_r(out_vertices, out_indices, out_expressions, nodes_stack, *node.r);
}

void BuildSceneMeshNode_impl(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	CSGExpressionGPUBuffer& out_expressions,
	NodesStack& nodes_stack,
	const TreeElementsLowLevel::Add& node)
{
	BuildSceneMeshNode_r(out_vertices, out_indices, out_expressions, nodes_stack, *node.l);
	BuildSceneMeshNode_r(out_vertices, out_indices, out_expressions, nodes_stack, *node.r);
}

void BuildSceneMeshNode_impl(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	CSGExpressionGPUBuffer& out_expressions,
	NodesStack& nodes_stack,
	const TreeElementsLowLevel::Sub& node)
{
	BuildSceneMeshNode_r(out_vertices, out_indices, out_expressions, nodes_stack, *node.l);
	BuildSceneMeshNode_r(out_vertices, out_indices, out_expressions, nodes_stack, *node.r);
}

void BuildSceneMeshNode_impl(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	CSGExpressionGPUBuffer& out_expressions,
	NodesStack& nodes_stack,
	const TreeElementsLowLevel::Leaf& node)
{
	const size_t start_offset= out_expressions.size();
	out_expressions.push_back(CSGExpressionGPUBufferType(node.surface_index));

	const size_t expression_size_offset= out_expressions.size();
	out_expressions.push_back(0); // Reserve place for size.

	const size_t expression_start_offset= out_expressions.size();
	out_expressions.push_back(CSGExpressionGPUBufferType(GPUCSGExpressionCodes::OneLeaf));

	const auto process_sub= [&](const CSGExpressionBuildResult res)
	{
		if(res == CSGExpressionBuildResult::Variable)
			out_expressions.push_back(CSGExpressionGPUBufferType(GPUCSGExpressionCodes::Sub));
		else if(res == CSGExpressionBuildResult::AlwaysZero){}
		else if(res == CSGExpressionBuildResult::AlwaysOne)
		{
			out_expressions.resize(expression_start_offset);
			out_expressions.push_back(CSGExpressionGPUBufferType(GPUCSGExpressionCodes::ZeroLeaf));
		}
		else SZV_ASSERT(false);
	};

	const auto process_mul= [&](const CSGExpressionBuildResult res)
	{
		if(res == CSGExpressionBuildResult::Variable)
			out_expressions.push_back(CSGExpressionGPUBufferType(GPUCSGExpressionCodes::Mul));
		else if(res == CSGExpressionBuildResult::AlwaysZero)
		{
			out_expressions.resize(expression_start_offset);
			out_expressions.push_back(CSGExpressionGPUBufferType(GPUCSGExpressionCodes::ZeroLeaf));
		}
		else if(res == CSGExpressionBuildResult::AlwaysOne) {}
		else SZV_ASSERT(false);
	};

	SZV_ASSERT(!nodes_stack.empty() && std::get_if<TreeElementsLowLevel::Leaf>(nodes_stack.back()) == &node);
	for(size_t i= nodes_stack.size() - 1u; i > 0u; --i)
	{
		const TreeElementsLowLevel::TreeElement* const el= nodes_stack[i - 1u];
		if(const auto add= std::get_if<TreeElementsLowLevel::Add>(el))
		{
			const bool this_is_left= add->l.get() == nodes_stack[i];
			process_sub(BUILDCSGExpression_r(out_expressions, node.bb, this_is_left ? *add->r : *add->l));
		}
		else if(const auto mul= std::get_if<TreeElementsLowLevel::Mul>(el))
		{
			const bool this_is_left= mul->l.get() == nodes_stack[i];
			process_mul(BUILDCSGExpression_r(out_expressions, node.bb, this_is_left ? *mul->r : *mul->l));
		}
		else if(const auto sub= std::get_if<TreeElementsLowLevel::Sub>(el))
		{
			const bool this_is_left= sub->l.get() == nodes_stack[i];
			if (this_is_left)
				process_sub(BUILDCSGExpression_r(out_expressions, node.bb, *sub->r));
			else
				process_mul(BUILDCSGExpression_r(out_expressions, node.bb, *sub->l));
		}
		else SZV_ASSERT(false);
	}

	out_expressions[expression_size_offset]= CSGExpressionGPUBufferType(out_expressions.size());

	AddCube(out_vertices, out_indices, node.bb, start_offset);
}

void BuildSceneMeshNode_r(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	CSGExpressionGPUBuffer& out_expressions,
	NodesStack& nodes_stack,
	const TreeElementsLowLevel::TreeElement& node)
{
	nodes_stack.push_back(&node);

	std::visit(
		[&](const auto& el)
		{
			BuildSceneMeshNode_impl(out_vertices, out_indices, out_expressions, nodes_stack, el);
		},
		node);

	nodes_stack.pop_back();
}

} // namespace

void BuildSceneMeshTree(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	CSGExpressionGPUBuffer& out_expressions,
	const TreeElementsLowLevel::TreeElement& root)
{
	NodesStack nodes_stack;
	BuildSceneMeshNode_r(out_vertices, out_indices, out_expressions, nodes_stack, root);
}

} // namespace SZV
