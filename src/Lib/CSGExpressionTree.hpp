#pragma once
#include "Vec.hpp"
#include <cstdint>
#include <variant>
#include <vector>

namespace SZV
{

namespace CSGTree
{

struct MulChain;
struct AddChain;
struct SubChain;
struct AddArray;
struct Ellipsoid;
struct Box;
struct Cylinder;
struct Cone;
struct Paraboloid;
struct Hyperboloid;
struct ParabolicCylinder;
struct HyperbolicCylinder;
struct HyperbolicParaboloid;

using CSGTreeNode= std::variant<
	MulChain,
	AddChain,
	SubChain,
	AddArray,
	Ellipsoid,
	Box,
	Cylinder,
	Cone,
	Paraboloid,
	Hyperboloid,
	ParabolicCylinder,
	HyperbolicCylinder,
	HyperbolicParaboloid>;

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

struct AddArray
{
	std::vector<CSGTreeNode> elements;
	uint8_t size[3];
	m_Vec3 step;
	m_Vec3 angles_deg;
};

struct Ellipsoid
{
	m_Vec3 center;
	m_Vec3 size;
	m_Vec3 angles_deg;
};

struct Box
{
	m_Vec3 center;
	m_Vec3 size;
	m_Vec3 angles_deg;
};

struct Cylinder
{
	m_Vec3 center;
	m_Vec3 size;
	m_Vec3 angles_deg;
};

struct Cone
{
	m_Vec3 center;
	m_Vec3 size;
	m_Vec3 angles_deg;
};

struct Paraboloid
{
	m_Vec3 center;
	m_Vec3 size;
	m_Vec3 angles_deg;
};

struct Hyperboloid
{
	m_Vec3 center;
	m_Vec3 size;
	m_Vec3 angles_deg;
	float focus_distance;
};

struct ParabolicCylinder
{
	m_Vec3 center;
	m_Vec3 size;
	m_Vec3 angles_deg;
};

struct HyperbolicCylinder
{
	m_Vec3 center;
	m_Vec3 size;
	m_Vec3 angles_deg;
	float focus_distance;
};

struct HyperbolicParaboloid
{
	m_Vec3 center;
	m_Vec3 angles_deg;
	float height;
	// TODO - add more parameters.
};

} // namespace CSGTree

} // namespac SZV
