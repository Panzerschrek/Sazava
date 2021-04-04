#include "../SDL2ViewerLib/Host.hpp"
#include "CentralWidget.hpp"
#include "CSGNodesTreeWidget.hpp"
#include "NewNodeListWidget.hpp"
#include <QtCore/QTimer>
#include <QtWidgets/QLabel>

namespace SZV
{

namespace
{

class CentralWidgetSDLVulkan final : public CentralWidgetBase
{
public:
	explicit CentralWidgetSDLVulkan(QWidget* const parent)
		: CentralWidgetBase(parent)
		, csg_tree_model_(host_.GetCSGTree())
		, layout_(this)
		, new_node_list_widget_(
			this,
			[this](CSGTree::CSGTreeNode node){ csg_nodes_tree_widget_.AddNode(std::move(node)); }
			)
		, csg_nodes_tree_widget_(csg_tree_model_, this)
	{
		connect(&timer_, &QTimer::timeout, this, &CentralWidgetSDLVulkan::Loop);
		timer_.start(20);

		layout_.addWidget(&new_node_list_widget_);

		const auto down_layout= new QHBoxLayout();
		down_layout->addWidget(&csg_nodes_tree_widget_, 1);
		down_layout->addWidget(new QLabel("Sorry, QtVulkan is not available", this), 4);

		layout_.insertLayout(1, down_layout);

		setLayout(&layout_);
	}

public: // ICentralWidget
	CSGTree::CSGTreeNode& GetCSGTreeRoot() override
	{
		return csg_tree_model_.GetRoot();
	}

	const CSGTree::CSGTreeNode& GetCSGTreeRoot() const override
	{
		return csg_tree_model_.GetRoot();
	}

private:
	void Loop()
	{
		if(host_.Loop())
			close();
	}

private:
	Host host_;
	QTimer timer_;
	CSGTreeModel csg_tree_model_;
	QVBoxLayout layout_;
	NewNodeListWidget new_node_list_widget_;
	CSGNodesTreeWidget csg_nodes_tree_widget_;
};

} // namespace

CentralWidgetBase* CreateCentralWidget(QWidget* const parent)
{
	return new CentralWidgetSDLVulkan(parent);
}

} // namespace SZV
