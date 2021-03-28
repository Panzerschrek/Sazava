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
struct Ellipsoid;
struct Box;
struct Cylinder;
struct Cone;
struct Paraboloid;
struct Hyperboloid;
struct HyperbolicParaboloid;

using CSGTreeNode= std::variant<
	MulChain,
	AddChain,
	SubChain,
	Ellipsoid,
	Box,
	Cylinder,
	Cone,
	Paraboloid,
	Hyperboloid,
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

struct Ellipsoid
{
	m_Vec3 center;
	m_Vec3 size;
	m_Vec3 normal;
	m_Vec3 binormal;
};

struct Box
{
	// Vectors must be normalized and perpendicular!
	m_Vec3 center;
	m_Vec3 size;
	m_Vec3 normal;
	m_Vec3 binormal;
};

struct Cylinder
{
	// Vectors must be normalized and perpendicular!
	m_Vec3 center;
	m_Vec3 size;
	m_Vec3 normal;
	m_Vec3 binormal;
};

struct Cone
{
	m_Vec3 center;
	m_Vec3 size;

	// Vectors must be normalized and perpendicular!
	// TODO - replace vectors with something else
	m_Vec3 normal; // main axis of the cone
	m_Vec3 binormal;
};

struct Paraboloid
{
	m_Vec3 center;
	m_Vec3 size;

	// Vectors must be normalized and perpendicular!
	// TODO - replace vectors with something else
	m_Vec3 normal; // main axis of the cone
	m_Vec3 binormal;
};

struct Hyperboloid
{
	m_Vec3 center;

	// Vectors must be normalized and perpendicular!
	// TODO - replace vectors with something else
	m_Vec3 normal; // main axis of the cone
	m_Vec3 binormal;
	float factor;
	float radius; // At zero
	float height;
};

struct HyperbolicParaboloid
{
	m_Vec3 center;

	// Vectors must be normalized and perpendicular!
	// TODO - replace vectors with something else
	m_Vec3 normal; // main axis of the cone
	m_Vec3 binormal;
	float height;
	// TODO - add more parameters.
};

} // namespace CSGTree

} // namespac SZV
