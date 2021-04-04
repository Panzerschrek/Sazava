#pragma once
#include "../Lib/CSGExpressionTree.hpp"
#include <QtWidgets/QWidget>

namespace SZV
{

class CentralWidgetBase : public QWidget
{
public:
	explicit CentralWidgetBase(QWidget* const parent) : QWidget(parent) {}

	virtual CSGTree::CSGTreeNode& GetCSGTreeRoot() = 0;
	virtual const CSGTree::CSGTreeNode& GetCSGTreeRoot() const = 0;
};

CentralWidgetBase* CreateCentralWidget(QWidget* parent);

} // namespace SZV
