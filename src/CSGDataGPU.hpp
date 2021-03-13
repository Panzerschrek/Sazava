#pragma once
#include "CSGExpressionTreeLowLevel.hpp"
#include <vector>

namespace SZV
{

struct SurfaceVertex
{
	float pos[3];
	float surface_description_offset; // Integer converted to float
};

using IndexType= uint16_t;

using VerticesVector= std::vector<SurfaceVertex>;
using IndicesVector= std::vector<IndexType>;

using CSGExpressionGPUBufferType= uint32_t;
using CSGExpressionGPUBuffer= std::vector<CSGExpressionGPUBufferType>;

void BuildSceneMeshTree(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	CSGExpressionGPUBuffer& out_expressions,
	const TreeElementsLowLevel::TreeElement& root);

} // namespace SZV
