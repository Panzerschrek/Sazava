#pragma once
#include "CSGExpressionTreeLowLevel.hpp"
#include <vector>

namespace SZV
{

struct SurfaceVertex
{
	float pos[3];
	float surface_description_offset;
};

using IndexType= uint16_t;

using VerticesVector= std::vector<SurfaceVertex>;
using IndicesVector= std::vector<IndexType>;

using GPUCSGExpressionBufferType= uint32_t;
using GPUCSGExpressionBuffer= std::vector<GPUCSGExpressionBufferType>;

enum class GPUCSGExpressionCodes : GPUCSGExpressionBufferType
{
	Mul= 100,
	Add= 101,
	Sub= 102,

	Leaf= 200,
	OneLeaf= 300,
	ZeroLeaf= 301,
};

void BuildSceneMeshTree(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	GPUCSGExpressionBuffer& out_expressions,
	const TreeElementsLowLevel::TreeElement& root);

} // namespace SZV
