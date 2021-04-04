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
		QJsonArray angles;
		angles.append(node.angles_deg.x);
		angles.append(node.angles_deg.y);
		angles.append(node.angles_deg.z);
		obj["angles"]= std::move(angles);
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

QJsonObject CSGTreeNodeToJson_impl(const CSGTree::ParabolicCylinder& node)
{
	return CSGTreeLeafNodeToJson(node, "parabolic_cylinder");
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

	QJsonArray angles;
	angles.append(node.angles_deg.x);
	angles.append(node.angles_deg.y);
	angles.append(node.angles_deg.z);
	obj["angles"]= std::move(angles);

	return obj;
}

QJsonObject CSGTreeNodeToJson(const CSGTree::CSGTreeNode& node)
{
	return std::visit([](const auto& n){ return CSGTreeNodeToJson_impl(n); }, node);
}

CSGTree::CSGTreeNode GetDummyCSGTreeNode()
{
	return CSGTree::AddChain();
}

CSGTree::CSGTreeNode JsonToCSGTreeNode(const QJsonObject& obj);

std::vector<CSGTree::CSGTreeNode> GetBranchNodeElements(const QJsonObject& obj)
{
	std::vector<CSGTree::CSGTreeNode> res;
	for(const QJsonValue el : obj["elements"].toArray())
		res.push_back(JsonToCSGTreeNode(el.toObject()));

	return res;
}

template<typename T>
void ReadLeafNodeElements(T& node, const QJsonObject& obj)
{
	{
		const QJsonArray center= obj["center"].toArray();
		node.center.x= float(center[0].toDouble());
		node.center.y= float(center[1].toDouble());
		node.center.z= float(center[2].toDouble());
	}
	{
		const QJsonArray size= obj["size"].toArray();
		node.size.x= float(size[0].toDouble());
		node.size.y= float(size[1].toDouble());
		node.size.z= float(size[2].toDouble());
	}
	{
		const QJsonArray angles= obj["angles"].toArray();
		node.angles_deg.x= float(angles[0].toDouble());
		node.angles_deg.y= float(angles[1].toDouble());
		node.angles_deg.z= float(angles[2].toDouble());
	}
}

CSGTree::CSGTreeNode JsonToCSGTreeNode(const QJsonObject& obj)
{
	const QString type= obj["type"].toString();
	if(type == "mul")
		return CSGTree::MulChain{ GetBranchNodeElements(obj) };
	if(type == "add")
		return CSGTree::AddChain{ GetBranchNodeElements(obj) };
	if(type == "sub")
		return CSGTree::SubChain{ GetBranchNodeElements(obj) };

	if(type == "ellipsoid")
	{
		CSGTree::Ellipsoid ellipsoid{};
		ReadLeafNodeElements(ellipsoid, obj);
		return ellipsoid;
	}
	if(type == "box")
	{
		CSGTree::Box box{};
		ReadLeafNodeElements(box, obj);
		return box;
	}
	if(type == "cylinder")
	{
		CSGTree::Cylinder cylinder{};
		ReadLeafNodeElements(cylinder, obj);
		return cylinder;
	}
	if(type == "cone")
	{
		CSGTree::Cone cone{};
		ReadLeafNodeElements(cone, obj);
		return cone;
	}
	if(type == "paraboloid")
	{
		CSGTree::Paraboloid paraboloid{};
		ReadLeafNodeElements(paraboloid, obj);
		return paraboloid;
	}
	if(type == "hyperboloid")
	{
		CSGTree::Hyperboloid hyperboloid{};
		ReadLeafNodeElements(hyperboloid, obj);
		hyperboloid.focus_distance= float(obj["focus_distance"].toDouble());
		return hyperboloid;
	}
	if(type == "parabolic_cylinder")
	{
		CSGTree::ParabolicCylinder parabolic_cylinder{};
		ReadLeafNodeElements(parabolic_cylinder, obj);
		return parabolic_cylinder;
	}
	if(type == "hyperbolic_paraboloid")
	{
		CSGTree::HyperbolicParaboloid node{};

		{
			const QJsonArray center= obj["center"].toArray();
			node.center.x= float(center[0].toDouble());
			node.center.y= float(center[1].toDouble());
			node.center.z= float(center[2].toDouble());
		}
		node.height= float(obj["height"].toDouble());
		{
			const QJsonArray angles= obj["angles"].toArray();
			node.angles_deg.x= float(angles[0].toDouble());
			node.angles_deg.y= float(angles[1].toDouble());
			node.angles_deg.z= float(angles[2].toDouble());
		}

		return node;
	}

	return GetDummyCSGTreeNode();
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
	QJsonDocument doc= QJsonDocument::fromJson(tree_serialized);

	if(!doc.isObject())
		return GetDummyCSGTreeNode();

	const QJsonObject header= doc.object();
	const QJsonObject root= header["root"].toObject();

	return JsonToCSGTreeNode(root);
}

} // namespace SZV
