#pragma once
#include "Vec.hpp"
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
struct Cone;

using CSGTreeNode= std::variant<
	MulChain,
	AddChain,
	SubChain,
	Sphere,
	Cube,
	Cylinder,
	Cone >;

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
	m_Vec3 center;
	float radius;
};

struct Cube
{
	m_Vec3 center;
	m_Vec3 size;
};

struct Cylinder
{
	m_Vec3 center;
	m_Vec3 normal;
	float radius;
};

struct Cone
{
	m_Vec3 center;

	// Vectors must be normalized and perpendicular!
	// TODO - replace vectors with something else
	m_Vec3 normal; // main axis of the cone
	m_Vec3 binormal;
};

} // namespace CSGTree

} // namespac SZV
