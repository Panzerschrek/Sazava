#include "CSGTreeModel.hpp"

namespace SZV
{

namespace
{

using ElementsVector= std::vector<CSGTree::CSGTreeNode>;
ElementsVector* GetElementsVectorImpl(CSGTree::MulChain& node) { return &node.elements; }
ElementsVector* GetElementsVectorImpl(CSGTree::AddChain& node) { return &node.elements; }
ElementsVector* GetElementsVectorImpl(CSGTree::SubChain& node) { return &node.elements; }
template<class T> ElementsVector* GetElementsVectorImpl(T&) { return nullptr; }
ElementsVector* GetElementsVector(CSGTree::CSGTreeNode& node)
{
	return std::visit(
		[&](auto& el)
		{
			return GetElementsVectorImpl(el);
		},
		node);
}

CSGTree::CSGTreeNode* FindParent(const CSGTree::CSGTreeNode& child, CSGTree::CSGTreeNode& current_node)
{
	return std::visit(
		[&](auto& el) -> CSGTree::CSGTreeNode*
		{
			if (const auto vec= GetElementsVectorImpl(el))
			{
				for (auto& vec_el : *vec)
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
QString GetElementTypeNameImpl(const CSGTree::Ellipsoid&) { return "ellipsoid"; }
QString GetElementTypeNameImpl(const CSGTree::Box&) { return "box"; }
QString GetElementTypeNameImpl(const CSGTree::Cylinder&) { return "cylinder"; }
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

CSGTreeModel::CSGTreeModel(CSGTree::CSGTreeNode& root)
	: root_(root)
{}

CSGTree::CSGTreeNode& CSGTreeModel::GetRoot()
{
	return root_;
}

const CSGTree::CSGTreeNode& CSGTreeModel::GetRoot() const
{
	return root_;
}

void CSGTreeModel::Reset(CSGTree::CSGTreeNode new_root)
{
	beginResetModel();
	root_= std::move(new_root);
	endResetModel();
}

void CSGTreeModel::DeleteNode(const QModelIndex& index)
{
	if(!index.isValid())
		return;

	const auto element_ptr= reinterpret_cast<CSGTree::CSGTreeNode*>(index.internalPointer());

	if(const auto parent= FindParent(*element_ptr, root_))
	{
		if(const auto vec= GetElementsVector(*parent))
		{
			const auto index_in_parent= int(element_ptr - vec->data());

			beginRemoveRows(CSGTreeModel::parent(index), index_in_parent, index_in_parent);
			vec->erase(vec->begin() + index_in_parent);
			endRemoveRows();
		}
	}
}

void CSGTreeModel::AddNode(const QModelIndex& index, CSGTree::CSGTreeNode node)
{
	if(!index.isValid())
		return;

	const auto element_ptr= reinterpret_cast<CSGTree::CSGTreeNode*>(index.internalPointer());

	if(const auto parent= FindParent(*element_ptr, root_))
	{
		if(const auto vec= GetElementsVector(*parent))
		{
			const auto index_in_parent= int(element_ptr - vec->data());
			// Reset model because indexes are invalidated after insertion into vector. TODO - maybe invalidate only small part of model?
			beginResetModel();
			vec->insert(vec->begin() + index_in_parent, std::move(node));
			endResetModel();
		}
	}
}

void CSGTreeModel::MoveUpNode(const QModelIndex& index)
{
	if(!index.isValid())
		return;

	const auto element_ptr= reinterpret_cast<CSGTree::CSGTreeNode*>(index.internalPointer());

	if(const auto parent= FindParent(*element_ptr, root_))
	{
		ElementsVector& vec= *GetElementsVector(*parent);

		const size_t index_in_parent= size_t(element_ptr - vec.data());
		if(index_in_parent > 0u)
		{
			std::swap(vec[index_in_parent - 1u], vec[index_in_parent]);

			const auto parent_index= CSGTreeModel::parent(index);
			emit dataChanged(
				CSGTreeModel::index(int(index_in_parent - 1), 0, parent_index),
				CSGTreeModel::index(int(index_in_parent    ), 0, parent_index));
		}
	}
}

void CSGTreeModel::MoveDownNode(const QModelIndex& index)
{
	if(!index.isValid())
		return;

	const auto element_ptr= reinterpret_cast<CSGTree::CSGTreeNode*>(index.internalPointer());

	if(const auto parent= FindParent(*element_ptr, root_))
	{
		ElementsVector& vec= *GetElementsVector(*parent);

		const size_t index_in_parent= size_t(element_ptr - vec.data());
		if(index_in_parent + 1u < vec.size())
		{
			std::swap(vec[index_in_parent], vec[index_in_parent + 1u]);

			const auto parent_index= CSGTreeModel::parent(index);
			emit dataChanged(
				CSGTreeModel::index(int(index_in_parent    ), 0, parent_index),
				CSGTreeModel::index(int(index_in_parent + 1), 0, parent_index));
		}
	}
}

QModelIndex CSGTreeModel::index(const int row, const int column, const QModelIndex& parent) const
{
	if(!parent.isValid())
		return createIndex(row, column, reinterpret_cast<uintptr_t>(&root_));

	const auto element_ptr= reinterpret_cast<CSGTree::CSGTreeNode*>(parent.internalPointer());
	if(const auto vec= GetElementsVector(*element_ptr))
		return createIndex(row, column, reinterpret_cast<uintptr_t>(&(*vec)[row]));
	return QModelIndex();
}

QModelIndex CSGTreeModel::parent(const QModelIndex& child) const
{
	if(!child.isValid())
		return QModelIndex();

	const auto element_ptr= reinterpret_cast<CSGTree::CSGTreeNode*>(child.internalPointer());
	const auto parent= FindParent(*element_ptr, const_cast<CSGTree::CSGTreeNode&>(root_));
	if(parent == nullptr)
		return QModelIndex();

	if(const auto parent_of_parent= FindParent(*parent, const_cast<CSGTree::CSGTreeNode&>(root_)))
	{
		if(const auto vec= GetElementsVector(*parent_of_parent))
			return createIndex(int(parent - vec->data()), 0, reinterpret_cast<uintptr_t>(parent));
	}

	return createIndex(0, 0, reinterpret_cast<uintptr_t>(parent));
}

int CSGTreeModel::rowCount(const QModelIndex& parent) const
{
	if(!parent.isValid())
		return 1;

	const auto element_ptr= reinterpret_cast<CSGTree::CSGTreeNode*>(parent.internalPointer());
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

	return GetElementTypeName(*ptr);
}

} // namespace SZV
