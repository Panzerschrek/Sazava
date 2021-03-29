#pragma once
#include "../Lib/CSGExpressionTree.hpp"
#include <QtCore/QAbstractItemModel>

namespace SZV
{

class CSGTreeModel final : public QAbstractItemModel
{
public:
	explicit CSGTreeModel(CSGTree::CSGTreeNode& root);

	void Reset(CSGTree::CSGTreeNode new_root);

	void DeleteNode(const QModelIndex& index);
	void AddNode(const QModelIndex& index, CSGTree::CSGTreeNode node);
	void MoveUpNode(const QModelIndex& index);
	void MoveDownNode(const QModelIndex& index);

public: // QAbstractItemModel
	QModelIndex index(int row, int column, const QModelIndex& parent) const override;
	QModelIndex parent(const QModelIndex& child) const override;
	int rowCount(const QModelIndex& parent) const override;
	int columnCount(const QModelIndex& parent) const override;
	QVariant data(const QModelIndex& index, int role) const override;

private:
	using Path= QVector<size_t>;

private:
	CSGTree::CSGTreeNode& root_;
};

} // namespace SZV
