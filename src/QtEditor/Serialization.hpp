#pragma once
#include "../Lib/CSGExpressionTree.hpp"
#include <QtCore/QByteArray>

namespace SZV
{

QByteArray SerializeCSGExpressionTree(const CSGTree::CSGTreeNode& root);
CSGTree::CSGTreeNode DeserializeCSGExpressionTree(const QByteArray& tree_serialized);

} // namespace SZV
