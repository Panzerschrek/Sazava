#pragma once
#include "Vec.hpp"
#include "CSGExpressionTree.hpp"
#include <memory>
#include <variant>

namespace SZV
{

struct BoundingBox
{
	m_Vec3 min;
	m_Vec3 max;
};

namespace TreeElementsLowLevel
{

struct Add;
struct Mul;
struct Sub;

struct Leaf
{
	size_t surface_index;
	BoundingBox bb;
};

using TreeElement= std::variant<
	Add,
	Mul,
	Sub,
	Leaf >;

struct Add
{
	std::unique_ptr<TreeElement> l;
	std::unique_ptr<TreeElement> r;
};

struct Mul
{
	std::unique_ptr<TreeElement> l;
	std::unique_ptr<TreeElement> r;
};

struct Sub
{
	std::unique_ptr<TreeElement> l;
	std::unique_ptr<TreeElement> r;
};

} // namespace TreeElementsLowLevel

struct GPUSurface
{
	// Surface quadratic equation parameters.
	float xx;
	float yy;
	float zz;
	float xy;
	float xz;
	float yz;
	float x;
	float y;
	float z;
	float k;
	float reserved[6];
};
static_assert(sizeof(GPUSurface) == sizeof(float) * 16, "Invalid size");

using GPUSurfacesVector= std::vector<GPUSurface>;

TreeElementsLowLevel::TreeElement BuildLowLevelTree(GPUSurfacesVector& out_surfaces, const CSGTree::CSGTreeNode& root);

} // namespace SZV
