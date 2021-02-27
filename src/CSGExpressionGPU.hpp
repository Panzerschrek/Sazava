#pragma once
#include "CSGExpressionTree.hpp"

namespace SZV
{

// Convert CSG tree into reverse polish notation for GPU.
using CSGExpressionGPU= std::vector<float>;
CSGExpressionGPU ConvertCSGTreeToGPUExpression(const CSGTree::CSGTreeNode& tree);

} // namespace SZV
