#include "CSGTreeModel.hpp"

namespace SZV
{

namespace
{

using ElementsVector= std::vector<CSGTree::CSGTreeNode>;
const ElementsVector* GetElementsVectorImpl(const CSGTree::MulChain& node) { return &node.elements; }
const ElementsVector* GetElementsVectorImpl(const CSGTree::AddChain& node) { return &node.elements; }
const ElementsVector* GetElementsVectorImpl(const CSGTree::SubChain& node) { return &node.elements; }
template<class T> const ElementsVector* GetElementsVectorImpl(const T&) { return nullptr; }
const ElementsVector* GetElementsVector(const CSGTree::CSGTreeNode& node)
{
	return std::visit(
		[&](const auto& el)
		{
			return GetElementsVectorImpl(el);
		},
		node);
}

const CSGTree::CSGTreeNode* FindParent(const CSGTree::CSGTreeNode& child, const CSGTree::CSGTreeNode& current_node)
{
	return std::visit(
		[&](const auto& el) -> const CSGTree::CSGTreeNode*
		{
			if (const auto vec= GetElementsVectorImpl(el))
			{
				for (const auto& vec_el : *vec)
				{
					if(&vec_el == &child)
						return &current_node;
					else if(const auto p= FindParent(child, vec_el))
						return p;
				}
			}

			return nullptr;
		},
		current_node);
}

QString GetElementTypeNameImpl(const CSGTree::MulChain&) { return "mul"; }
QString GetElementTypeNameImpl(const CSGTree::AddChain&) { return "add"; }
QString GetElementTypeNameImpl(const CSGTree::SubChain&) { return "sub"; }
QString GetElementTypeNameImpl(const CSGTree::Sphere&) { return "shpere"; }
QString GetElementTypeNameImpl(const CSGTree::Ellipsoid&) { return "ellipsoid"; }
QString GetElementTypeNameImpl(const CSGTree::Cube&) { return "cube"; }
QString GetElementTypeNameImpl(const CSGTree::Cylinder&) { return "cylinder"; }
QString GetElementTypeNameImpl(const CSGTree::EllipticCylinder&) { return "elliptic cylinder"; }
QString GetElementTypeNameImpl(const CSGTree::Cone&) { return "cone"; }
QString GetElementTypeNameImpl(const CSGTree::Paraboloid&) { return "paraboloid"; }
QString GetElementTypeNameImpl(const CSGTree::Hyperboloid&) { return "hyperboloid"; }
QString GetElementTypeNameImpl(const CSGTree::HyperbolicParaboloid&) { return "hyperbolic paraboloid"; }
QString GetElementTypeName(const CSGTree::CSGTreeNode& node)
{
	return std::visit(
		[&](const auto& el)
		{
			return GetElementTypeNameImpl(el);
		},
		node);
}

} // namespace

void CSGTreeModel::SetTree(CSGTree::CSGTreeNode tree)
{
	beginResetModel();
	root_= std::move(tree);
	endResetModel();
}

QModelIndex CSGTreeModel::index(const int row, const int column, const QModelIndex& parent) const
{
	const auto element_ptr= parent.isValid() ? reinterpret_cast<const CSGTree::CSGTreeNode*>(parent.internalPointer()) : &root_;
	if(const auto vec= GetElementsVector(*element_ptr))
		return createIndex(row, column, reinterpret_cast<uintptr_t>(&(*vec)[row]));
	return QModelIndex();
}

QModelIndex CSGTreeModel::parent(const QModelIndex& child) const
{
	if(!child.isValid())
		return QModelIndex();

	const auto element_ptr= reinterpret_cast<const CSGTree::CSGTreeNode*>(child.internalPointer());
	const auto parent= FindParent(*element_ptr, root_);
	if(parent == nullptr)
		return QModelIndex();

	const auto vec= GetElementsVector(*parent);
	const auto index_in_parent= element_ptr - vec->data();

	return createIndex(int(index_in_parent), 0, reinterpret_cast<uintptr_t>(parent));
}

int CSGTreeModel::rowCount(const QModelIndex& parent) const
{
	const auto element_ptr= parent.isValid() ? reinterpret_cast<const CSGTree::CSGTreeNode*>(parent.internalPointer()) : &root_;
	if (const auto vec= GetElementsVector(*element_ptr))
		return int(vec->size());
	return 0;
}

int CSGTreeModel::columnCount(const QModelIndex& parent) const
{
	(void)parent;
	return 1;
}

QVariant CSGTreeModel::data(const QModelIndex &index, const int role) const
{
	if(role != Qt::DisplayRole)
		return QVariant();

	const auto ptr= reinterpret_cast<const CSGTree::CSGTreeNode*>(index.internalPointer());
	if(ptr == nullptr)
		return QVariant();

	return GetElementTypeName(*ptr) + QString("  ") + QString::number(index.internalId());
}

} // namespace SZV
