#pragma once
// This header contains representation of structs for shader.
// If this code changed, corresponding shader must be changed too!

namespace SZV
{

enum class ExpressionElementType
{
	// Operations. Have no parameters.
	End= 0,
	Mul= 1,
	Add= 2,
	Sub= 3,

	// Objects. Have parameters (specific count for each element).
	Sphere= 101,
	Cube= 102,
	Cylinder= 103,
};

namespace ExpressionElements
{

struct Sphere
{
	float center[3];
	float radius;
};

struct Cube
{
	float center[3];
	float half_size[3];
};

struct Cylinder
{
	float center[3];
	float normal[3];
	float radius;
};

} // namespace ExpressionElements

} // namespace SZV
