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
struct Ellipsoid;
struct Cube;
struct Cylinder;
struct EllipticCylinder;
struct Cone;
struct Paraboloid;

using CSGTreeNode= std::variant<
	MulChain,
	AddChain,
	SubChain,
	Sphere,
	Ellipsoid,
	Cube,
	Cylinder,
	EllipticCylinder,
	Cone,
	Paraboloid >;

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

struct Ellipsoid
{
	m_Vec3 center;
	m_Vec3 radius;
	m_Vec3 normal;
	m_Vec3 binormal;
};

struct Cube
{
	m_Vec3 center;
	m_Vec3 size;
};

struct Cylinder
{
	// Vectors must be normalized and perpendicular!
	m_Vec3 center;
	m_Vec3 normal;
	m_Vec3 binormal;
	float radius;
	float height;
};

struct EllipticCylinder
{
	// Vectors must be normalized and perpendicular!
	m_Vec3 center;
	m_Vec3 normal;
	m_Vec3 binormal;
	m_Vec2 radius;
	float height;
};

struct Cone
{
	m_Vec3 center;

	// Vectors must be normalized and perpendicular!
	// TODO - replace vectors with something else
	m_Vec3 normal; // main axis of the cone
	m_Vec3 binormal;
	float angle;
	float height;
};

struct Paraboloid
{
	m_Vec3 center;

	// Vectors must be normalized and perpendicular!
	// TODO - replace vectors with something else
	m_Vec3 normal; // main axis of the cone
	m_Vec3 binormal;
	float factor;
	float height;
};

} // namespace CSGTree

} // namespac SZV
