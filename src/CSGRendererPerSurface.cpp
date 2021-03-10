#include "CSGRendererPerSurface.hpp"
#include "Assert.hpp"

namespace SZV
{

namespace
{

namespace Shaders
{
#include "shaders/surface.vert.h"
#include "shaders/surface.frag.h"


} // namespace Shaders

struct Uniforms
{
	m_Mat4 view_matrix;
	m_Vec3 cam_pos; float reserved;
	float dir_to_sun_normalized[4];
	float sun_color[4];
	float ambient_light_color[4];
};

struct SurfaceVertex
{
	float pos[3];
	float surface_index;
};

using IndexType= uint16_t;

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

using VerticesVector= std::vector<SurfaceVertex>;
using IndicesVector= std::vector<IndexType>;
using GPUSurfacesVector= std::vector<GPUSurface>;

namespace TreeElementsLowLevel
{

struct Add;
struct Mul;
struct Sub;
struct Leaf;
struct OneLeaf{};
struct ZeroLeaf{};

using TreeElement= std::variant<
	Add,
	Mul,
	Sub,
	Leaf,
	OneLeaf,
	ZeroLeaf>;

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

struct Leaf
{
	size_t surface_index;
	m_Vec3 bb_min;
	m_Vec3 bb_max;
};

} // namespace TreeElementsLowLevel

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

GPUSurface TransformSurface_impl(
	const GPUSurface& s,
	const m_Vec3& shift,
	const m_Vec3& x_vec, const m_Vec3& y_vec, const m_Vec3& z_vec)
{
	GPUSurface r{};

	r.xx=
		s.xx * (x_vec.x * x_vec.x) +
		s.yy * (y_vec.x * y_vec.x) +
		s.zz * (z_vec.x * z_vec.x) +
		s.xy * x_vec.x * y_vec.x +
		s.xz * x_vec.x * z_vec.x +
		s.yz * y_vec.x * z_vec.x;
	r.yy=
		s.xx * (x_vec.y * x_vec.y) +
		s.yy * (y_vec.y * y_vec.y) +
		s.zz * (z_vec.y * z_vec.y) +
		s.xy * x_vec.y * y_vec.y +
		s.xz * x_vec.y * z_vec.y +
		s.yz * y_vec.y * z_vec.y;
	r.zz=
		s.xx * (x_vec.z * x_vec.z) +
		s.yy * (y_vec.z * y_vec.z) +
		s.zz * (z_vec.z * z_vec.z) +
		s.xy * x_vec.z * y_vec.z +
		s.xz * x_vec.z * z_vec.z +
		s.yz * y_vec.z * z_vec.z;

	r.xy=
		2.0f * (s.xx * x_vec.x * x_vec.y + s.yy * y_vec.x * y_vec.y + s.zz * z_vec.x * z_vec.y) +
		s.xy * (x_vec.x * y_vec.y + y_vec.x * x_vec.y) +
		s.xz * (x_vec.x * z_vec.y + z_vec.x * x_vec.y) +
		s.yz * (y_vec.x * z_vec.y + z_vec.x * y_vec.y);
	r.xz=
		2.0f * (s.xx * x_vec.x * x_vec.z + s.yy * y_vec.x * y_vec.z + s.zz * z_vec.x * z_vec.z) +
		s.xy * (x_vec.x * y_vec.z + y_vec.x * x_vec.z) +
		s.xz * (x_vec.x * z_vec.z + z_vec.x * x_vec.z) +
		s.yz * (y_vec.x * z_vec.z + z_vec.x * y_vec.z);
	r.yz=
		2.0f * (s.xx * x_vec.y * x_vec.z + s.yy * y_vec.y * y_vec.z + s.zz * z_vec.y * z_vec.z) +
		s.xy * (x_vec.y * y_vec.z + y_vec.y * x_vec.z) +
		s.xz * (x_vec.y * z_vec.z + z_vec.y * x_vec.z) +
		s.yz * (y_vec.y * z_vec.z + z_vec.y * y_vec.z);

	r.x=
		2.0f * (s.xx * x_vec.x * shift.x + s.yy * y_vec.x * shift.y + s.zz * z_vec.x * shift.z) +
		s.xy * (x_vec.x * shift.y + y_vec.x * shift.x) +
		s.xz * (x_vec.x * shift.z + z_vec.x * shift.x) +
		s.yz * (y_vec.x * shift.z + z_vec.x * shift.y) +
		(s.x * x_vec.x + s.y * y_vec.x + s.z * z_vec.x);
	r.y=
		2.0f * (s.xx * x_vec.y * shift.x + s.yy * y_vec.y * shift.y + s.zz * z_vec.y * shift.z) +
		s.xy * (x_vec.y * shift.y + y_vec.y * shift.x) +
		s.xz * (x_vec.y * shift.z + z_vec.y * shift.x) +
		s.yz * (y_vec.y * shift.z + z_vec.y * shift.y) +
		(s.x * x_vec.y + s.y * y_vec.y + s.z * z_vec.y);
	r.z=
		2.0f * (s.xx * x_vec.z * shift.x + s.yy * y_vec.z * shift.y + s.zz * z_vec.z * shift.z) +
		s.xy * (x_vec.z * shift.y + y_vec.z * shift.x) +
		s.xz * (x_vec.z * shift.z + z_vec.z * shift.x) +
		s.yz * (y_vec.z * shift.z + z_vec.z * shift.y) +
		(s.x * x_vec.z + s.y * y_vec.z + s.z * z_vec.z);

	r.k=
		s.xx * shift.x * shift.x + s.yy * shift.y * shift.y + s.zz * shift.z * shift.z +
		s.xy * shift.x * shift.y +
		s.xz * shift.x * shift.z +
		s.yz * shift.y * shift.z +
		s.x * shift.x + s.y * shift.y + s.z * shift.z +
		s.k;

	return r;
}

// Warning!
// normal and binormal vectors must be perpendicular and must have identity length!
GPUSurface TransformSurface(const GPUSurface& s, const m_Vec3& center, const m_Vec3& normal, const m_Vec3& binormal)
{
	const m_Vec3 tangent= mVec3Cross(normal, binormal);

	return
		TransformSurface_impl(
			s,
			-m_Vec3(mVec3Dot(center, binormal), mVec3Dot(center, tangent), mVec3Dot(center, normal)),
			binormal,
			tangent,
			normal);
}

TreeElementsLowLevel::TreeElement BuildLowLevelTree_r(GPUSurfacesVector& out_surfaces, const CSGTree::CSGTreeNode& node);

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::MulChain& node)
{
	TreeElementsLowLevel::Mul mul;
	mul.l= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[0]));
	mul.r= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[1]));

	for (size_t i= 2u; i < node.elements.size(); ++i)
	{
		TreeElementsLowLevel::Mul mul_element;
		mul_element.l= std::make_unique<TreeElementsLowLevel::TreeElement>(std::move(mul));
		mul_element.r= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[i]));
		mul= std::move(mul_element);
	}

	return std::move(mul);
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::AddChain& node)
{
	TreeElementsLowLevel::Add add;
	add.l= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[0]));
	add.r= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[1]));

	for (size_t i= 2u; i < node.elements.size(); ++i)
	{
		TreeElementsLowLevel::Add add_element;
		add_element.l= std::make_unique<TreeElementsLowLevel::TreeElement>(std::move(add));
		add_element.r= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[i]));
		add= std::move(add_element);
	}

	return std::move(add);
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::SubChain& node)
{
	TreeElementsLowLevel::Sub sub;
	sub.l= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[0]));
	sub.r= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[1]));

	for (size_t i= 2u; i < node.elements.size(); ++i)
	{
		TreeElementsLowLevel::Sub sub_element;
		sub_element.l= std::make_unique<TreeElementsLowLevel::TreeElement>(std::move(sub));
		sub_element.r= std::make_unique<TreeElementsLowLevel::TreeElement>(BuildLowLevelTree_r(out_surfaces, node.elements[i]));
		sub= std::move(sub_element);
	}

	return std::move(sub);
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::Sphere& node)
{
	GPUSurface surface{};
	surface.xx= surface.yy= surface.zz= 1.0f;
	surface.k= - node.radius * node.radius;
	surface= TransformSurface(surface, node.center, m_Vec3(0.0f, 0.0f, 1.0f), m_Vec3(1.0f, 0.0f, 0.0f));

	const size_t surface_index= out_surfaces.size();
	out_surfaces.push_back(surface);

	TreeElementsLowLevel::Leaf leaf;
	leaf.surface_index= surface_index;
	leaf.bb_min= node.center - m_Vec3(node.radius, node.radius, node.radius);
	leaf.bb_max= node.center + m_Vec3(node.radius, node.radius, node.radius);
	return leaf;
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::Cube& node)
{
	// Represent three pairs of parallel planes of cube using three quadratic surfaces.
	const m_Vec3 normal(0.0f, 0.0f, 1.0f);
	const m_Vec3 binormal(1.0, 0.0f, 0.0f);
	const m_Vec3 bb_min= node.center - node.size * 0.5f;
	const m_Vec3 bb_max= node.center + node.size * 0.5f;
	const size_t surface_index= out_surfaces.size();

	{
		GPUSurface surface{};
		surface.xx= 1.0f;
		surface.k= -0.25f * node.size.x * node.size.x;
		surface= TransformSurface(surface, node.center, normal, binormal);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.yy= 1.0f;
		surface.k= -0.25f * node.size.y * node.size.y;
		surface= TransformSurface(surface, node.center, normal, binormal);
		out_surfaces.push_back(surface);
	}
	{
		GPUSurface surface{};
		surface.zz= 1.0f;
		surface.k= -0.25f * node.size.z * node.size.z;
		surface= TransformSurface(surface, node.center, normal, binormal);
		out_surfaces.push_back(surface);
	}

	TreeElementsLowLevel::Leaf leafs[3];
	for (size_t i= 0u; i < 3u; ++i)
	{
		leafs[i].surface_index= surface_index + i;
		leafs[i].bb_min= bb_min;
		leafs[i].bb_max= bb_max;
	}

	return TreeElementsLowLevel::Mul
	{
		std::make_unique<TreeElementsLowLevel::TreeElement>(
			TreeElementsLowLevel::Mul
			{
				std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[0]),
				std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[1]),
			}),
		std::make_unique<TreeElementsLowLevel::TreeElement>(leafs[2]),
	};
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::Cylinder& node)
{
	GPUSurface surface{};
	surface.xx= surface.yy= 1.0f;
	surface.k= - node.radius * node.radius;
	surface= TransformSurface(surface, node.center, node.normal, node.binormal);

	const size_t surface_index= out_surfaces.size();
	out_surfaces.push_back(surface);

	// TODO - calculate bounding box more precisely
	const float circular_radius= std::sqrt(node.radius * node.radius + 0.25f * node.height * node.height);

	TreeElementsLowLevel::Leaf leaf;
	leaf.surface_index= surface_index;
	leaf.bb_min= node.center - m_Vec3(circular_radius, circular_radius, circular_radius);
	leaf.bb_max= node.center + m_Vec3(circular_radius, circular_radius, circular_radius);
	return leaf;
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::Cone& node)
{
	const float k= std::tan(node.angle * 0.5f);

	GPUSurface surface{};
	surface.xx= surface.yy= 1.0f;
	surface.zz= -k * k;
	surface= TransformSurface(surface, node.center, node.normal, node.binormal);

	const size_t surface_index= out_surfaces.size();
	out_surfaces.push_back(surface);

	// TODO - calculate bounding box more precisely
	const float circular_radius= std::sqrt(node.height * node.height + node.height * node.height * k * k);

	TreeElementsLowLevel::Leaf leaf;
	leaf.surface_index= surface_index;
	leaf.bb_min= node.center - m_Vec3(circular_radius, circular_radius, circular_radius);
	leaf.bb_max= node.center + m_Vec3(circular_radius, circular_radius, circular_radius);
	return leaf;
}

TreeElementsLowLevel::TreeElement BuildLowLevelTreeNode_impl(GPUSurfacesVector& out_surfaces, const CSGTree::Paraboloid& node)
{
	GPUSurface surface{};
	surface.xx= surface.yy= 1.0f;
	surface.z= -node.factor;
	surface= TransformSurface(surface, node.center, node.normal, node.binormal);

	const size_t surface_index= out_surfaces.size();
	out_surfaces.push_back(surface);

	// TODO - calculate bounding box more precisely
	const float circular_radius= std::sqrt(node.height * node.height + std::sqrt(node.height) / node.factor);

	TreeElementsLowLevel::Leaf leaf;
	leaf.surface_index= surface_index;
	leaf.bb_min= node.center - m_Vec3(circular_radius, circular_radius, circular_radius);
	leaf.bb_max= node.center + m_Vec3(circular_radius, circular_radius, circular_radius);
	return leaf;
}

TreeElementsLowLevel::TreeElement BuildLowLevelTree_r(GPUSurfacesVector& out_surfaces, const CSGTree::CSGTreeNode& node)
{
	return std::visit(
		[&](const auto& el)
		{
			return BuildLowLevelTreeNode_impl(out_surfaces, el);
		},
		node);
}

void AddCube(VerticesVector& out_vertices, IndicesVector& out_indices, const m_Vec3& center, const m_Vec3 half_size, const size_t surface_idnex)
{
	static const m_Vec3 cube_vertices[8]=
	{
		{ 1.0f,  1.0f,  1.0f }, { -1.0f,  1.0f,  1.0f },
		{ 1.0f, -1.0f,  1.0f }, { -1.0f, -1.0f,  1.0f },
		{ 1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f, -1.0f },
	};

	static const IndexType cube_indices[12 * 3]=
	{
		0, 1, 5,  0, 5, 4,
		0, 4, 6,  0, 6, 2,
		4, 5, 7,  4, 7, 6,
		0, 3, 1,  0, 2, 3,
		2, 7, 3,  2, 6, 7,
		1, 3, 7,  1, 7, 5,
	};

	const size_t start_index= out_vertices.size();
	for(const m_Vec3& cube_vertex : cube_vertices)
	{
		const SurfaceVertex v
		{
			{
				cube_vertex.x * half_size.x + center.x,
				cube_vertex.y * half_size.y + center.y,
				cube_vertex.z * half_size.z + center.z,
			},
			float(surface_idnex),
		};
		out_vertices.push_back(v);
	}

	for(const size_t cube_index : cube_indices)
		out_indices.push_back(IndexType(cube_index + start_index));
}

void BUILDCSGExpression_r(GPUCSGExpressionBuffer& out_expression, const TreeElementsLowLevel::TreeElement& node);


void BUILDCSGExpressionNode_impl(GPUCSGExpressionBuffer& out_expression, const TreeElementsLowLevel::Mul& node)
{
	BUILDCSGExpression_r(out_expression, *node.l);
	BUILDCSGExpression_r(out_expression, *node.r);
	out_expression.push_back(GPUCSGExpressionBufferType(GPUCSGExpressionCodes::Mul));
	// TODO - process zero/one nodes
}

void BUILDCSGExpressionNode_impl(GPUCSGExpressionBuffer& out_expression, const TreeElementsLowLevel::Add& node)
{
	BUILDCSGExpression_r(out_expression, *node.l);
	BUILDCSGExpression_r(out_expression, *node.r);
	out_expression.push_back(GPUCSGExpressionBufferType(GPUCSGExpressionCodes::Add));
	// TODO - process zero/one nodes
}

void BUILDCSGExpressionNode_impl(GPUCSGExpressionBuffer& out_expression, const TreeElementsLowLevel::Sub& node)
{
	BUILDCSGExpression_r(out_expression, *node.l);
	BUILDCSGExpression_r(out_expression, *node.r);
	out_expression.push_back(GPUCSGExpressionBufferType(GPUCSGExpressionCodes::Sub));
	// TODO - process zero/one nodes
}

void BUILDCSGExpressionNode_impl(GPUCSGExpressionBuffer& out_expression, const TreeElementsLowLevel::Leaf& node)
{
	out_expression.push_back(GPUCSGExpressionBufferType(GPUCSGExpressionCodes::Leaf));
	out_expression.push_back(GPUCSGExpressionBufferType(node.surface_index));
}

void BUILDCSGExpressionNode_impl(GPUCSGExpressionBuffer& out_expression, const TreeElementsLowLevel::OneLeaf& )
{
	out_expression.push_back(GPUCSGExpressionBufferType(GPUCSGExpressionCodes::ZeroLeaf));
}

void BUILDCSGExpressionNode_impl(GPUCSGExpressionBuffer& out_expression, const TreeElementsLowLevel::ZeroLeaf& )
{
	out_expression.push_back(GPUCSGExpressionBufferType(GPUCSGExpressionCodes::OneLeaf));
}

void BUILDCSGExpression_r(GPUCSGExpressionBuffer& out_expression, const TreeElementsLowLevel::TreeElement& node)
{
	std::visit(
		[&](const auto& el)
		{
			BUILDCSGExpressionNode_impl(out_expression, el);
		},
		node);
}

void BuildSceneMeshNode_r(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	GPUCSGExpressionBuffer& out_expressions,
	const TreeElementsLowLevel::TreeElement& root,
	const TreeElementsLowLevel::TreeElement& node);

void BuildSceneMeshNode_impl(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	GPUCSGExpressionBuffer& out_expressions,
	const TreeElementsLowLevel::TreeElement& root,
	const TreeElementsLowLevel::Mul& node)
{
	BuildSceneMeshNode_r(out_vertices, out_indices, out_expressions, root, *node.l);
	BuildSceneMeshNode_r(out_vertices, out_indices, out_expressions, root, *node.r);
}

void BuildSceneMeshNode_impl(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	GPUCSGExpressionBuffer& out_expressions,
	const TreeElementsLowLevel::TreeElement& root,
	const TreeElementsLowLevel::Add& node)
{
	BuildSceneMeshNode_r(out_vertices, out_indices, out_expressions, root, *node.l);
	BuildSceneMeshNode_r(out_vertices, out_indices, out_expressions, root, *node.r);
}

void BuildSceneMeshNode_impl(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	GPUCSGExpressionBuffer& out_expressions,
	const TreeElementsLowLevel::TreeElement& root,
	const TreeElementsLowLevel::Sub& node)
{
	BuildSceneMeshNode_r(out_vertices, out_indices, out_expressions, root, *node.l);
	BuildSceneMeshNode_r(out_vertices, out_indices, out_expressions, root, *node.r);
}

void BuildSceneMeshNode_impl(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	GPUCSGExpressionBuffer& out_expressions,
	const TreeElementsLowLevel::TreeElement& root,
	const TreeElementsLowLevel::Leaf& node)
{
	SZV_UNUSED(out_expressions);
	SZV_UNUSED(root);

	AddCube(
		out_vertices,
		out_indices,
		(node.bb_min + node.bb_max) * 0.5f,
		(node.bb_max - node.bb_min) * 0.5f,
		node.surface_index);
}

void BuildSceneMeshNode_impl(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	GPUCSGExpressionBuffer& out_expressions,
	const TreeElementsLowLevel::TreeElement& root,
	const TreeElementsLowLevel::OneLeaf& node)
{
	SZV_UNUSED(out_vertices);
	SZV_UNUSED(out_indices);
	SZV_UNUSED(out_expressions);
	SZV_UNUSED(root);
	SZV_UNUSED(node);
}

void BuildSceneMeshNode_impl(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	GPUCSGExpressionBuffer& out_expressions,
	const TreeElementsLowLevel::TreeElement& root,
	const TreeElementsLowLevel::ZeroLeaf& node)
{
	SZV_UNUSED(out_vertices);
	SZV_UNUSED(out_indices);
	SZV_UNUSED(out_expressions);
	SZV_UNUSED(root);
	SZV_UNUSED(node);
}

void BuildSceneMeshNode_r(
	VerticesVector& out_vertices,
	IndicesVector& out_indices,
	GPUCSGExpressionBuffer& out_expressions,
	const TreeElementsLowLevel::TreeElement& root,
	const TreeElementsLowLevel::TreeElement& node)
{
	std::visit(
		[&](const auto& el)
		{
			BuildSceneMeshNode_impl(out_vertices, out_indices, out_expressions, root, el);
		},
		node);
}

} // namespace

CSGRendererPerSurface::CSGRendererPerSurface(WindowVulkan& window_vulkan)
	: vk_device_(window_vulkan.GetVulkanDevice())
	, tonemapper_(window_vulkan)
{
	const vk::PhysicalDeviceMemoryProperties& memory_properties= window_vulkan.GetMemoryProperties();

	{ // Create surfaces data buffer
		const uint32_t buffer_size= 65536u; // Maximum size for vkCmdUpdateBuffer
		surfaces_buffer_size_= buffer_size;

		surfaces_data_buffer_gpu_=
			vk_device_.createBufferUnique(
				vk::BufferCreateInfo(
					vk::BufferCreateFlags(),
					buffer_size,
					vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst));

		const vk::MemoryRequirements buffer_memory_requirements=
			vk_device_.getBufferMemoryRequirements(*surfaces_data_buffer_gpu_);

		vk::MemoryAllocateInfo vk_memory_allocate_info(buffer_memory_requirements.size);
		for(uint32_t i= 0u; i < memory_properties.memoryTypeCount; ++i)
		{
			if((buffer_memory_requirements.memoryTypeBits & (1u << i)) != 0 &&
				(memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) != vk::MemoryPropertyFlags())
				vk_memory_allocate_info.memoryTypeIndex= i;
		}

		surfaces_data_buffer_memory_= vk_device_.allocateMemoryUnique(vk_memory_allocate_info);
		vk_device_.bindBufferMemory(*surfaces_data_buffer_gpu_, *surfaces_data_buffer_memory_, 0u);
	}
	{ // Create surfaces data buffer
		const uint32_t buffer_size= 65536u; // Maximum size for vkCmdUpdateBuffer
		expressions_buffer_size_= buffer_size;

		expressions_data_buffer_gpu_=
			vk_device_.createBufferUnique(
				vk::BufferCreateInfo(
					vk::BufferCreateFlags(),
					buffer_size,
					vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst));

		const vk::MemoryRequirements buffer_memory_requirements=
			vk_device_.getBufferMemoryRequirements(*expressions_data_buffer_gpu_);

		vk::MemoryAllocateInfo vk_memory_allocate_info(buffer_memory_requirements.size);
		for(uint32_t i= 0u; i < memory_properties.memoryTypeCount; ++i)
		{
			if((buffer_memory_requirements.memoryTypeBits & (1u << i)) != 0 &&
				(memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) != vk::MemoryPropertyFlags())
				vk_memory_allocate_info.memoryTypeIndex= i;
		}

		expressions_data_buffer_memory_= vk_device_.allocateMemoryUnique(vk_memory_allocate_info);
		vk_device_.bindBufferMemory(*expressions_data_buffer_gpu_, *expressions_data_buffer_memory_, 0u);
	}
	{ // Create descriptor set layout
		const vk::DescriptorSetLayoutBinding descriptor_set_layout_bindings[2]
		{
			{
				0u,
				vk::DescriptorType::eStorageBuffer,
				1u,
				vk::ShaderStageFlagBits::eFragment,
				nullptr,
			},
			{
				1u,
				vk::DescriptorType::eStorageBuffer,
				1u,
				vk::ShaderStageFlagBits::eFragment,
				nullptr,
			},
		};

		descriptor_set_layout_=
			vk_device_.createDescriptorSetLayoutUnique(
					vk::DescriptorSetLayoutCreateInfo(
						vk::DescriptorSetLayoutCreateFlags(),
						uint32_t(std::size(descriptor_set_layout_bindings)), descriptor_set_layout_bindings));
	}
	{ // Create pipeline layout
		const vk::PushConstantRange vk_push_constant_ranges[1]
		{
			{vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0u, sizeof(Uniforms)},
		};

		pipeline_layout_=
			vk_device_.createPipelineLayoutUnique(
				vk::PipelineLayoutCreateInfo(
					vk::PipelineLayoutCreateFlags(),
					1u, &*descriptor_set_layout_,
					uint32_t(std::size(vk_push_constant_ranges)), vk_push_constant_ranges));
	}
	{ // Create pipeline.
		shader_vert_=
			vk_device_.createShaderModuleUnique(
				vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
				sizeof(Shaders::surface_vert),
				Shaders::surface_vert));

		shader_frag_=
			vk_device_.createShaderModuleUnique(
				vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
				sizeof(Shaders::surface_frag),
				Shaders::surface_frag));

		const vk::PipelineShaderStageCreateInfo vk_shader_stage_create_info[]
		{
			{
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eVertex,
				*shader_vert_,
				"main"
			},
			{
				vk::PipelineShaderStageCreateFlags(),
				vk::ShaderStageFlagBits::eFragment,
				*shader_frag_,
				"main"
			},
		};

		const vk::VertexInputBindingDescription vk_vertex_input_binding_description(
			0u,
			sizeof(SurfaceVertex),
			vk::VertexInputRate::eVertex);

		const vk::VertexInputAttributeDescription vk_vertex_input_attribute_description[1]
		{
			{0u, 0u, vk::Format::eR32G32B32A32Sfloat, offsetof(SurfaceVertex, pos)},
		};

		const vk::PipelineVertexInputStateCreateInfo vk_pipiline_vertex_input_state_create_info(
			vk::PipelineVertexInputStateCreateFlags(),
			1u, &vk_vertex_input_binding_description,
			uint32_t(std::size(vk_vertex_input_attribute_description)), vk_vertex_input_attribute_description);

		const vk::PipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_state_create_info(
			vk::PipelineInputAssemblyStateCreateFlags(),
			vk::PrimitiveTopology::eTriangleList);

		const vk::Extent2D viewport_size= tonemapper_.GetFramebufferSize();
		const vk::Viewport vk_viewport(0.0f, 0.0f, float(viewport_size.width), float(viewport_size.height), 0.0f, 1.0f);
		const vk::Rect2D vk_scissor(vk::Offset2D(0, 0), viewport_size);

		const vk::PipelineViewportStateCreateInfo vk_pipieline_viewport_state_create_info(
			vk::PipelineViewportStateCreateFlags(),
			1u, &vk_viewport,
			1u, &vk_scissor);

		const vk::PipelineRasterizationStateCreateInfo vk_pipilane_rasterization_state_create_info(
			vk::PipelineRasterizationStateCreateFlags(),
			VK_FALSE,
			VK_FALSE,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eFront,
			vk::FrontFace::eClockwise,
			VK_FALSE, 0.0f, 0.0f, 0.0f,
			1.0f);

		const vk::PipelineMultisampleStateCreateInfo vk_pipeline_multisample_state_create_info;

		const vk::PipelineDepthStencilStateCreateInfo vk_pipeline_depth_state_create_info(
				vk::PipelineDepthStencilStateCreateFlags(),
				VK_TRUE,
				VK_TRUE,
				vk::CompareOp::eLess,
				VK_FALSE,
				VK_FALSE,
				vk::StencilOpState(),
				vk::StencilOpState(),
				0.0f,
				1.0f);

		const vk::PipelineColorBlendAttachmentState pipeline_color_blend_attachment_state(
			VK_FALSE,
			vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
			vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

		const vk::PipelineColorBlendStateCreateInfo vk_pipeline_color_blend_state_create_info(
			vk::PipelineColorBlendStateCreateFlags(),
			VK_FALSE,
			vk::LogicOp::eCopy,
			1u, &pipeline_color_blend_attachment_state);

		pipeline_=
			vk_device_.createGraphicsPipelineUnique(
				nullptr,
				vk::GraphicsPipelineCreateInfo(
					vk::PipelineCreateFlags(),
					uint32_t(std::size(vk_shader_stage_create_info)), vk_shader_stage_create_info,
					&vk_pipiline_vertex_input_state_create_info,
					&vk_pipeline_input_assembly_state_create_info,
					nullptr,
					&vk_pipieline_viewport_state_create_info,
					&vk_pipilane_rasterization_state_create_info,
					&vk_pipeline_multisample_state_create_info,
					&vk_pipeline_depth_state_create_info,
					&vk_pipeline_color_blend_state_create_info,
					nullptr,
					*pipeline_layout_,
					tonemapper_.GetMainRenderPass(),
					0u));
	}

	{ // Create descriptor pool
		const vk::DescriptorPoolSize vk_descriptor_pool_sizes[1]
		{
			{
				vk::DescriptorType::eStorageBuffer,
				2u // global storage buffers
			},
		};

		descriptor_pool_=
			vk_device_.createDescriptorPoolUnique(
				vk::DescriptorPoolCreateInfo(
					vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
					1u, // max sets.
					uint32_t(std::size(vk_descriptor_pool_sizes)), vk_descriptor_pool_sizes));
	}
	{ // Create and fill descriptor set
		descriptor_set_=
			std::move(
			vk_device_.allocateDescriptorSetsUnique(
				vk::DescriptorSetAllocateInfo(
					*descriptor_pool_,
					1u, &*descriptor_set_layout_)).front());

		const vk::DescriptorBufferInfo descriptor_buffer_info_surfaces(
			*surfaces_data_buffer_gpu_,
			0u,
			surfaces_buffer_size_);

		const vk::DescriptorBufferInfo descriptor_buffer_info_expressions(
			*expressions_data_buffer_gpu_,
			0u,
			expressions_buffer_size_);

		vk_device_.updateDescriptorSets(
			{
				{
					*descriptor_set_,
					0u,
					0u,
					1u,
					vk::DescriptorType::eStorageBuffer,
					nullptr,
					&descriptor_buffer_info_surfaces,
					nullptr,
				},
				{
					*descriptor_set_,
					1u,
					0u,
					1u,
					vk::DescriptorType::eStorageBuffer,
					nullptr,
					&descriptor_buffer_info_expressions,
					nullptr,
				},
			},
			{});
	}

	{ // Create vertex buffer.
		vertex_buffer_vertices_= 65536u / sizeof(SurfaceVertex);

		vertex_buffer_=
			vk_device_.createBufferUnique(
				vk::BufferCreateInfo(
					vk::BufferCreateFlags(),
					vertex_buffer_vertices_ * sizeof(SurfaceVertex),
					vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst));

		const vk::MemoryRequirements buffer_memory_requirements= vk_device_.getBufferMemoryRequirements(*vertex_buffer_);

			vk::MemoryAllocateInfo vk_memory_allocate_info(buffer_memory_requirements.size);
			for(uint32_t i= 0u; i < memory_properties.memoryTypeCount; ++i)
			{
				if((buffer_memory_requirements.memoryTypeBits & (1u << i)) != 0 &&
					(memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) != vk::MemoryPropertyFlags())
					vk_memory_allocate_info.memoryTypeIndex= i;
			}

			vertex_buffer_memory_= vk_device_.allocateMemoryUnique(vk_memory_allocate_info);
			vk_device_.bindBufferMemory(*vertex_buffer_, *vertex_buffer_memory_, 0u);
	}
	{ // Create index buffer.
		index_buffer_indeces_= 65536u / sizeof(IndexType);

		index_buffer_=
				vk_device_.createBufferUnique(
					vk::BufferCreateInfo(
						vk::BufferCreateFlags(),
						index_buffer_indeces_ * sizeof(IndexType),
						vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst));

			const vk::MemoryRequirements buffer_memory_requirements= vk_device_.getBufferMemoryRequirements(*index_buffer_);

			vk::MemoryAllocateInfo vk_memory_allocate_info(buffer_memory_requirements.size);
			for(uint32_t i= 0u; i < memory_properties.memoryTypeCount; ++i)
			{
				if((buffer_memory_requirements.memoryTypeBits & (1u << i)) != 0 &&
					(memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) != vk::MemoryPropertyFlags())
					vk_memory_allocate_info.memoryTypeIndex= i;
			}

			index_buffer_memory_= vk_device_.allocateMemoryUnique(vk_memory_allocate_info);
			vk_device_.bindBufferMemory(*index_buffer_, *index_buffer_memory_, 0u);
	}
}

CSGRendererPerSurface::~CSGRendererPerSurface()
{
	// Sync before destruction.
	vk_device_.waitIdle();
}

void CSGRendererPerSurface::BeginFrame(
	const vk::CommandBuffer command_buffer,
	const CameraController& camera_controller,
	const CSGTree::CSGTreeNode& csg_tree)
{
	VerticesVector vertices;
	IndicesVector indices;
	GPUSurfacesVector surfaces;
	GPUCSGExpressionBuffer expressions;
	const auto low_level_tree= BuildLowLevelTree_r(surfaces, csg_tree);
	BuildSceneMeshNode_r(vertices, indices, expressions, low_level_tree, low_level_tree);

	expressions.push_back(0);
	BUILDCSGExpression_r(expressions, low_level_tree);
	expressions[0]= GPUCSGExpressionBufferType(expressions.size());

	command_buffer.updateBuffer(
		*vertex_buffer_,
		0u,
		vk::DeviceSize(sizeof(SurfaceVertex) * vertices.size()),
		vertices.data());

	command_buffer.updateBuffer(
		*index_buffer_,
		0u,
		vk::DeviceSize(sizeof(IndexType) * indices.size()),
		indices.data());

	command_buffer.updateBuffer(
		*surfaces_data_buffer_gpu_,
		0u,
		surfaces.size() * sizeof(GPUSurface),
		surfaces.data());

	command_buffer.updateBuffer(
		*expressions_data_buffer_gpu_,
		0u,
		expressions.size() * sizeof(GPUCSGExpressionBufferType),
		expressions.data());

	tonemapper_.DoMainPass(
		command_buffer,
		[&]
		{
			Draw(command_buffer, camera_controller, indices.size());
		});
}

void CSGRendererPerSurface::EndFrame(const vk::CommandBuffer command_buffer)
{
	tonemapper_.EndFrame(command_buffer);
}

void CSGRendererPerSurface::Draw(const vk::CommandBuffer command_buffer, const CameraController& camera_controller, const size_t index_count)
{
	Uniforms uniforms{};
	uniforms.view_matrix= camera_controller.CalculateFullViewMatrix();
	uniforms.cam_pos= camera_controller.GetCameraPosition();

	m_Vec3 dir_to_sun= m_Vec3( 0.2f, 0.3f, 0.4f );
	dir_to_sun*= dir_to_sun.GetInvLength();

	uniforms.dir_to_sun_normalized[0]= dir_to_sun.x;
	uniforms.dir_to_sun_normalized[1]= dir_to_sun.y;
	uniforms.dir_to_sun_normalized[2]= dir_to_sun.z;
	uniforms.sun_color[0]= 1.0f * 3.0f;
	uniforms.sun_color[1]= 0.9f * 3.0f;
	uniforms.sun_color[2]= 0.8f * 3.0f;
	uniforms.ambient_light_color[0]= 0.3f;
	uniforms.ambient_light_color[1]= 0.3f;
	uniforms.ambient_light_color[2]= 0.4f;

	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_);

	command_buffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		*pipeline_layout_,
		0u,
		1u, &*descriptor_set_,
		0u, nullptr);

	command_buffer.pushConstants(
		*pipeline_layout_,
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		0u,
		sizeof(uniforms),
		&uniforms);

	const vk::DeviceSize offsets= 0u;
	command_buffer.bindVertexBuffers(0u, 1u, &*vertex_buffer_, &offsets);
		command_buffer.bindIndexBuffer(*index_buffer_, 0u, sizeof(IndexType) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);

	command_buffer.drawIndexed(uint32_t(index_count), 1u, 0u, 0u, 0u);
}

} // namespace SZV
