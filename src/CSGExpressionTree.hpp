#pragma once
#include <variant>
#include <vector>

namespace SZV
{

namespace CSGTree
{

struct MulChain;
struct AddChain;
struct SubChain;
struct Sphere;
struct Cube;
struct Cylinder;

using CSGTreeNode= std::variant<
	MulChain,
	AddChain,
	SubChain,
	Sphere,
	Cube,
	Cylinder >;

struct MulChain
{
	std::vector<CSGTreeNode> elements;
};

struct AddChain
{
	std::vector<CSGTreeNode> elements;
};

struct SubChain
{
	std::vector<CSGTreeNode> elements;
};

struct Sphere
{
	float center[3];
	float radius;
};

struct Cube
{
	float center[3];
	float size[3];
};

struct Cylinder
{
	float center[3];
	float normal[3];
	float radius;
};

} // namespace CSGTree

} // namespac SZV
