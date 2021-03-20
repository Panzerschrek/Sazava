#pragma once
#include "../Lib/CSGExpressionTree.hpp"
#include <QtCore/QAbstractItemModel>

namespace SZV
{

class CSGTreeModel final : public QAbstractItemModel
{
public:
	void SetTree(CSGTree::CSGTreeNode tree);

public: // QAbstractItemModel
	QModelIndex index(int row, int column, const QModelIndex& parent) const override;
	QModelIndex parent(const QModelIndex &child) const override;
	int rowCount(const QModelIndex& parent) const override;
	int columnCount(const QModelIndex& parent) const override;
	QVariant data(const QModelIndex& index, int role) const override;

private:
	using Path= QVector<size_t>;

private:
	CSGTree::CSGTreeNode root_;
};

} // namespace SZV
