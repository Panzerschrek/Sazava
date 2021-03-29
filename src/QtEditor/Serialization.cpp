#include "Serialization.hpp"
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace SZV
{

namespace
{

QJsonObject CSGTreeNodeToJson(const CSGTree::CSGTreeNode& node);

template<typename T>
QJsonObject CSGTreeBranchNodeToJson(const T& node, const char* const type)
{
	QJsonObject obj;
	obj["type"]= type;

	QJsonArray elements;
	for(const CSGTree::CSGTreeNode& el : node.elements)
		elements.append(CSGTreeNodeToJson(el));

	obj["elements"]= std::move(elements);
	return obj;
}

template<typename T>
QJsonObject CSGTreeLeafNodeToJson(const T& node, const char* const type)
{
	QJsonObject obj;
	obj["type"]= type;

	{
		QJsonArray center;
		center.append(node.center.x);
		center.append(node.center.y);
		center.append(node.center.z);
		obj["center"]= std::move(center);
	}
	{
		QJsonArray size;
		size.append(node.size.x);
		size.append(node.size.y);
		size.append(node.size.z);
		obj["size"]= std::move(size);
	}
	{
		QJsonArray normal;
		normal.append(node.normal.x);
		normal.append(node.normal.y);
		normal.append(node.normal.z);
		obj["normal"]= std::move(normal);
	}
	{
		QJsonArray binormal;
		binormal.append(node.binormal.x);
		binormal.append(node.binormal.y);
		binormal.append(node.binormal.z);
		obj["binormal"]= std::move(binormal);
	}
	return obj;
}

QJsonObject CSGTreeNodeToJson_impl(const CSGTree::MulChain& node)
{
	return CSGTreeBranchNodeToJson(node, "mul");
}

QJsonObject CSGTreeNodeToJson_impl(const CSGTree::AddChain& node)
{
	return CSGTreeBranchNodeToJson(node, "add");
}

QJsonObject CSGTreeNodeToJson_impl(const CSGTree::SubChain& node)
{
	return CSGTreeBranchNodeToJson(node, "sub");
}

QJsonObject CSGTreeNodeToJson_impl(const CSGTree::Ellipsoid& node)
{
	return CSGTreeLeafNodeToJson(node, "ellipsoid");
}

QJsonObject CSGTreeNodeToJson_impl(const CSGTree::Box& node)
{
	return CSGTreeLeafNodeToJson(node, "box");
}

QJsonObject CSGTreeNodeToJson_impl(const CSGTree::Cylinder& node)
{
	return CSGTreeLeafNodeToJson(node, "cylinder");
}

QJsonObject CSGTreeNodeToJson_impl(const CSGTree::Cone& node)
{
	return CSGTreeLeafNodeToJson(node, "cone");
}

QJsonObject CSGTreeNodeToJson_impl(const CSGTree::Paraboloid& node)
{
	return CSGTreeLeafNodeToJson(node, "paraboloid");
}

QJsonObject CSGTreeNodeToJson_impl(const CSGTree::Hyperboloid& node)
{
	QJsonObject obj= CSGTreeLeafNodeToJson(node, "hyperboloid");
	obj["focus_distance"]= node.focus_distance;
	return obj;
}

QJsonObject CSGTreeNodeToJson_impl(const CSGTree::HyperbolicParaboloid& node)
{
	QJsonObject obj;
	obj["type"]= "hyperbolic_paraboloid";

	QJsonArray center;
	center.append(node.center.x);
	center.append(node.center.y);
	center.append(node.center.z);
	obj["center"]= std::move(center);

	obj["height"]= node.height;

	QJsonArray normal;
	normal.append(node.normal.x);
	normal.append(node.normal.y);
	normal.append(node.normal.z);
	obj["normal"]= std::move(normal);

	QJsonArray binormal;
	binormal.append(node.binormal.x);
	binormal.append(node.binormal.y);
	binormal.append(node.binormal.z);
	obj["binormal"]= std::move(binormal);

	return obj;
}

QJsonObject CSGTreeNodeToJson(const CSGTree::CSGTreeNode& node)
{
	return std::visit([](const auto& n){ return CSGTreeNodeToJson_impl(n); }, node);
}

} // namespace

QByteArray SerializeCSGExpressionTree(const CSGTree::CSGTreeNode& root)
{
	QJsonObject header;
	header["type"]= "csg_tree_root";
	header["root"]= CSGTreeNodeToJson(root);

	const QJsonDocument doc(header);
	return doc.toJson(QJsonDocument::Indented);
}

CSGTree::CSGTreeNode DeserializeCSGExpressionTree(const QByteArray& tree_serialized)
{
	// TODO
	(void)tree_serialized;
	return CSGTree::MulChain();
}

} // namespace SZV
