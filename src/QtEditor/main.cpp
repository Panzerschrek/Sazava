#include  "../SDL2ViewerLib/Host.hpp"
#include "../Lib/Log.hpp"
#include "CSGTreeModel.hpp"
#include "CSGTreeNodeEditWidget.hpp"
#include "NewNodeListWidget.hpp"
#include "Serialization.hpp"
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QVulkanWindow>

namespace SZV
{

namespace
{

m_Vec3 GetNodePos(const CSGTree::CSGTreeNode& node);

template<typename T>
m_Vec3 GetNodePosImpl(const T& t){ return t.center; }

m_Vec3 GetNodePosImpl(const CSGTree::MulChain& node)
{
	if(!node.elements.empty())
		return GetNodePos(node.elements.front());
	return m_Vec3(0.0f, 0.0f, 0.0f);
}

m_Vec3 GetNodePosImpl(const CSGTree::AddChain& node)
{
	if(!node.elements.empty())
		return GetNodePos(node.elements.front());
	return m_Vec3(0.0f, 0.0f, 0.0f);
}

m_Vec3 GetNodePosImpl(const CSGTree::SubChain& node)
{
	if(!node.elements.empty())
		return GetNodePos(node.elements.front());
	return m_Vec3(0.0f, 0.0f, 0.0f);
}

m_Vec3 GetNodePos(const CSGTree::CSGTreeNode& node)
{
	return std::visit([](const auto& n){ return GetNodePosImpl(n); }, node);
}

m_Vec3 GetNodeSize(const CSGTree::CSGTreeNode& node);

template<typename T>
m_Vec3 GetNodeSizeImpl(const T& t){ return t.size; }

m_Vec3 GetNodeSizeImpl(const CSGTree::MulChain& node)
{
	if(!node.elements.empty())
		return GetNodeSize(node.elements.front());
	return m_Vec3(1.0f, 1.0f, 1.0f);
}

m_Vec3 GetNodeSizeImpl(const CSGTree::AddChain& node)
{
	if(!node.elements.empty())
		return GetNodeSize(node.elements.front());
	return m_Vec3(1.0f, 1.0f, 1.0f);
}

m_Vec3 GetNodeSizeImpl(const CSGTree::SubChain& node)
{
	if(!node.elements.empty())
		return GetNodeSize(node.elements.front());
	return m_Vec3(1.0f, 1.0f, 1.0f);
}

m_Vec3 GetNodeSizeImpl(const CSGTree::HyperbolicParaboloid&)
{
	return m_Vec3(1.0f, 1.0f, 1.0f);
}

m_Vec3 GetNodeSize(const CSGTree::CSGTreeNode& node)
{
	return std::visit([](const auto& n){ return GetNodeSizeImpl(n); }, node);
}

m_Vec3 GetNodeAngles(const CSGTree::CSGTreeNode& node);

template<typename T>
m_Vec3 GetNodeAnglesImpl(const T& t){ return t.angles_deg; }

m_Vec3 GetNodeAnglesImpl(const CSGTree::MulChain& node)
{
	if(!node.elements.empty())
		return GetNodeAngles(node.elements.front());
	return m_Vec3(0.0f, 0.0f, 0.0f);
}

m_Vec3 GetNodeAnglesImpl(const CSGTree::AddChain& node)
{
	if(!node.elements.empty())
		return GetNodeAngles(node.elements.front());
	return m_Vec3(0.0f, 0.0f, 0.0f);
}

m_Vec3 GetNodeAnglesImpl(const CSGTree::SubChain& node)
{
	if(!node.elements.empty())
		return GetNodeAngles(node.elements.front());
	return m_Vec3(0.0f, 0.0f, 0.0f);
}

m_Vec3 GetNodeAngles(const CSGTree::CSGTreeNode& node)
{
	return std::visit([](const auto& n){ return GetNodeAnglesImpl(n); }, node);
}

void SetNodePosImpl(CSGTree::MulChain&, const m_Vec3&){}
void SetNodePosImpl(CSGTree::AddChain&, const m_Vec3&){}
void SetNodePosImpl(CSGTree::SubChain&, const m_Vec3&){}

template<typename T> void SetNodePosImpl(T& node, const m_Vec3& pos){ node.center= pos; }

void SetNodePos(CSGTree::CSGTreeNode& node, const m_Vec3& pos)
{
	std::visit([&](auto& n){ SetNodePosImpl(n, pos); }, node);
}

void SetNodeSizeImpl(CSGTree::MulChain&, const m_Vec3&){}
void SetNodeSizeImpl(CSGTree::AddChain&, const m_Vec3&){}
void SetNodeSizeImpl(CSGTree::SubChain&, const m_Vec3&){}
void SetNodeSizeImpl(CSGTree::HyperbolicParaboloid&, const m_Vec3&){}

template<typename T> void SetNodeSizeImpl(T& node, const m_Vec3& size){ node.size= size; }

void SetNodeSize(CSGTree::CSGTreeNode& node, const m_Vec3& size)
{
	std::visit([&](auto& n){ SetNodeSizeImpl(n, size); }, node);
}

void SetNodeAnglesImpl(CSGTree::MulChain&, const m_Vec3&){}
void SetNodeAnglesImpl(CSGTree::AddChain&, const m_Vec3&){}
void SetNodeAnglesImpl(CSGTree::SubChain&, const m_Vec3&){}
void SetNodeAnglesImpl(CSGTree::HyperbolicParaboloid&, const m_Vec3&){}

template<typename T> void SetNodeAnglesImpl(T& node, const m_Vec3& angles_deg){ node.angles_deg= angles_deg; }

void SetNodeAngles(CSGTree::CSGTreeNode& node, const m_Vec3& angles_deg)
{
	std::visit([&](auto& n){ SetNodeAnglesImpl(n, angles_deg); }, node);
}

class CSGNodesTreeWidget final : public QWidget
{
public:
	CSGNodesTreeWidget(CSGTreeModel& csg_tree_model, QWidget* const parent)
		: QWidget(parent), csg_tree_model_(csg_tree_model)
	{
		csg_tree_view_.setHeaderHidden(true);
		csg_tree_view_.setModel(&csg_tree_model_);
		csg_tree_view_.setSelectionBehavior(QAbstractItemView::SelectRows);
		csg_tree_view_.setSelectionMode(QAbstractItemView::SingleSelection);
		csg_tree_view_.setContextMenuPolicy(Qt::CustomContextMenu);
		connect(
			&csg_tree_view_,
			&QAbstractItemView::customContextMenuRequested,
			this,
			&CSGNodesTreeWidget::OnContextMenu);

		connect(
			csg_tree_view_.selectionModel(),
			&QItemSelectionModel::selectionChanged,
			this,
			&CSGNodesTreeWidget::OnSelectionChanged);

		setLayout(&layout_);
		layout_.addWidget(&csg_tree_view_);
	}

	void AddNode(CSGTree::CSGTreeNode node_template)
	{
		const auto index= csg_tree_view_.currentIndex();
		if(!index.isValid())
			return;

		const auto& node= *reinterpret_cast<CSGTree::CSGTreeNode*>(index.internalPointer());

		if(node.index() == node_template.index())
			node_template= node; // If new node have same type - copy all params.

		const m_Vec3 current_pos= GetNodePos(node);
		const m_Vec3 current_size= GetNodeSize(node);
		const m_Vec3 current_angles= GetNodeAngles(node);

		// Shift new node relative to old node to shoi it in different location.
		const m_Vec3 pos_shifted= current_pos + current_size;

		SetNodePos(node_template, pos_shifted);
		SetNodeSize(node_template, current_size);
		SetNodeAngles(node_template, current_angles);
		csg_tree_model_.AddNode(index, std::move(node_template));
	}

private:
	void OnContextMenu(const QPoint& p)
	{
		const auto menu= new QMenu(this);

		menu->addAction("delete", this, &CSGNodesTreeWidget::OnDeleteNode);
		menu->addAction("move up", this, &CSGNodesTreeWidget::OnMoveUpNode);
		menu->addAction("move down", this, &CSGNodesTreeWidget::OnMoveDownNode);

		menu->popup(csg_tree_view_.viewport()->mapToGlobal(p));
	}

	void OnDeleteNode()
	{
		csg_tree_model_.DeleteNode(csg_tree_view_.currentIndex());
		OnNodeActivated(csg_tree_view_.currentIndex());
	}

	void OnMoveUpNode()
	{
		csg_tree_model_.MoveUpNode(csg_tree_view_.currentIndex());
		OnNodeActivated(csg_tree_view_.currentIndex());
	}

	void OnMoveDownNode()
	{
		csg_tree_model_.MoveDownNode(csg_tree_view_.currentIndex());
		OnNodeActivated(csg_tree_view_.currentIndex());
	}

	void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
	{
		Q_UNUSED(deselected);
		if(!selected.empty())
		{
			const auto range= selected.front();
			if(!range.isEmpty())
				return OnNodeActivated(range.indexes().front());
		}
		OnNodeActivated(QModelIndex());
	}

	void OnNodeActivated(const QModelIndex& index)
	{
		if(edit_widget_ != nullptr)
		{
			layout_.removeWidget(edit_widget_);
			delete edit_widget_;
			edit_widget_= nullptr;
		}

		if(!index.isValid())
			return;

		auto node= reinterpret_cast<CSGTree::CSGTreeNode*>(index.internalPointer());
		edit_widget_= new CSGTreeNodeEditWidget(*node, this);
		layout_.addWidget(edit_widget_);
	}

private:
	QVBoxLayout layout_;
	QTreeView csg_tree_view_;
	CSGTreeModel& csg_tree_model_;
	CSGTreeNodeEditWidget* edit_widget_= nullptr;
};


CSGTree::CSGTreeNode GetTestCSGTree()
{
	return CSGTree::AddChain
	{ {
		CSGTree::Box{ { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
	} };
}

class VulkanRenderer final : public QVulkanWindowRenderer, public I_WindowVulkan
{
public:
	explicit VulkanRenderer(
		QVulkanWindow& window,
		const CSGTree::CSGTreeNode& csg_tree_root,
		const InputState& input_state)
		: window_(window)
		, csg_tree_root_(csg_tree_root)
		, input_state_(input_state)
		, camera_controller_(1.0f)
		, prev_tick_time_(Clock::now())
	{}

	void initResources() override
	{}

	void initSwapChainResources() override
	{
		if(csg_renderer_ != nullptr)
			csg_renderer_= nullptr;
		csg_renderer_= std::make_unique<CSGRenderer>(*this);
	}

	void releaseSwapChainResources() override
	{
		csg_renderer_= nullptr;
	}

	void releaseResources() override
	{
		csg_renderer_= nullptr;
	}

	void startNextFrame() override
	{
		if(csg_renderer_ == nullptr)
			return;

		UpdateCamera();

		vk::CommandBuffer command_buffer = window_.currentCommandBuffer();
		csg_renderer_->BeginFrame(command_buffer, camera_controller_, csg_tree_root_);

		const vk::ClearValue clear_value[]
		{
			{ vk::ClearColorValue(std::array<float,4>{0.0f, 0.0f, 0.0f, 0.0f}) },
			{ vk::ClearDepthStencilValue(1.0f, 0u) },
		};

		command_buffer.beginRenderPass(
			vk::RenderPassBeginInfo(
				window_.defaultRenderPass(),
				window_.currentFramebuffer(),
				vk::Rect2D(vk::Offset2D(0, 0), GetViewportSize()),
				2u, clear_value),
			vk::SubpassContents::eInline);

		csg_renderer_->EndFrame(command_buffer);

		command_buffer.endRenderPass();

		window_.frameReady();

		// Request update to schedule next repaint.
		window_.requestUpdate();
	}

public: // I_WindowVulkan
	vk::Device GetVulkanDevice() const override
	{
		return window_.device();
	}

	vk::Extent2D GetViewportSize() const override
	{
		const QSize s= window_.swapChainImageSize();
		return vk::Extent2D(uint32_t(s.width()), uint32_t(s.height()));
	}

	uint32_t GetQueueFamilyIndex() const override
	{
		return 0u; // TODO
	}

	vk::RenderPass GetRenderPass() const override
	{
		auto render_pass= window_.defaultRenderPass();
		return render_pass;
	}

	bool HasDepthBuffer() const override
	{
		return true;
	}

	vk::PhysicalDeviceMemoryProperties GetMemoryProperties() const
	{
		return GetPhysicalDevice().getMemoryProperties();
	}

	vk::PhysicalDevice GetPhysicalDevice() const override
	{
		return window_.physicalDevice();
	}

private:
	void UpdateCamera()
	{
		const QSize s= window_.swapChainImageSize();
		camera_controller_.SetAspect(float(s.width()) / float(s.height()));

		const Clock::time_point tick_start_time= Clock::now();
		const auto dt= tick_start_time - prev_tick_time_;
		prev_tick_time_ = tick_start_time;

		const float dt_s= float(dt.count()) * float(Clock::duration::period::num) / float(Clock::duration::period::den);
		camera_controller_.Update(dt_s, input_state_);
	}

	InputState GetInputState()
	{
		InputState s;

		return s;
	}

private:
	QVulkanWindow& window_;
	const CSGTree::CSGTreeNode& csg_tree_root_;
	const InputState& input_state_;
	CameraController camera_controller_;
	std::unique_ptr<CSGRenderer> csg_renderer_;

	using Clock= std::chrono::steady_clock;
	Clock::time_point prev_tick_time_;
};

class VulkanWindow final : public QVulkanWindow
{
public:
	VulkanWindow(QWindow* const parent, const CSGTree::CSGTreeNode& csg_tree_root)
		: QVulkanWindow(parent), csg_tree_root_(csg_tree_root)
	{}

	QVulkanWindowRenderer *createRenderer() override
	{
		return new VulkanRenderer(*this, csg_tree_root_, input_state_);
	}

private:
	void keyPressEvent(QKeyEvent* const event) override
	{
		event->accept();
		input_state_.keyboard[ size_t(TranslateKey(event->key())) ]= true;
	}

	void keyReleaseEvent(QKeyEvent* const event) override
	{
		event->accept();
		input_state_.keyboard[ size_t(TranslateKey(event->key())) ]= false;
	}

	void focusOutEvent(QFocusEvent* const event) override
	{
		event->accept();
		input_state_.keyboard.reset();
	}

private:
	SystemEventTypes::KeyCode TranslateKey(const int key)
	{
		using KC= SystemEventTypes::KeyCode;
		switch(key)
		{
		case Qt::Key_Up: return KC::Up;
		case Qt::Key_Down: return KC::Down;
		case Qt::Key_Left: return KC::Left;
		case Qt::Key_Right: return KC::Right;
		case::Qt::Key_Space: return KC::Space;
		}

		if(key >= Qt::Key_A && key <= Qt::Key_Z)
			return KC(int(KC::A) + key - Qt::Key_A);
		if(key >= Qt::Key_0 && key <= Qt::Key_9)
			return KC(int(KC::K0) + key - Qt::Key_0);

		return KC::Unknown;
	}

private:
	InputState input_state_{};
	const CSGTree::CSGTreeNode& csg_tree_root_;
};

class CentralWidget final : public QWidget
{
public:
	explicit CentralWidget(QWidget* const parent)
		: QWidget(parent)
		, csg_tree_root_(GetTestCSGTree())
		, csg_tree_model_(csg_tree_root_)
		, layout_(this)
		, new_node_list_widget_(
			this,
			[this](CSGTree::CSGTreeNode node){ csg_nodes_tree_widget_.AddNode(std::move(node)); }
			)
		, csg_nodes_tree_widget_(csg_tree_model_, this)
	{
		vulkan_instance_.setFlags(QVulkanInstance::NoDebugOutputRedirect);
		vulkan_instance_.create();
		vulkan_window_= new VulkanWindow(this->windowHandle(), csg_tree_root_);
		vulkan_window_->setVulkanInstance(&vulkan_instance_);

		layout_.addWidget(&new_node_list_widget_);

		const auto down_layout= new QHBoxLayout();
		down_layout->addWidget(&csg_nodes_tree_widget_, 1);
		down_layout->addWidget(QWidget::createWindowContainer(vulkan_window_, this), 4);

		layout_.insertLayout(1, down_layout);

		setLayout(&layout_);
	}

	CSGTree::CSGTreeNode& GetCSGTreeRoot()
	{
		return csg_tree_model_.GetRoot();
	}

	const CSGTree::CSGTreeNode& GetCSGTreeRoot() const
	{
		return csg_tree_model_.GetRoot();
	}

private:
	QVulkanInstance vulkan_instance_;
	VulkanWindow* vulkan_window_= nullptr;

	CSGTree::CSGTreeNode csg_tree_root_;
	CSGTreeModel csg_tree_model_;
	QVBoxLayout layout_;
	NewNodeListWidget new_node_list_widget_;
	CSGNodesTreeWidget csg_nodes_tree_widget_;
};

class MainWindow final : public QMainWindow
{
public:
	explicit MainWindow()
		: central_widget_(this)
	{
		const auto menu_bar = new QMenuBar(this);
		const auto file_menu = menu_bar->addMenu("&File");
		file_menu->addAction("&Open", this, &MainWindow::OnOpen);
		file_menu->addAction("&Save", this, &MainWindow::OnSave);
		file_menu->addAction("&Quit", this, &QWidget::close);
		setMenuBar(menu_bar);

		setCentralWidget(&central_widget_);
	}

private:
	void OnOpen()
	{
		const QString open_path= QFileDialog::getOpenFileName(this, "Sazava - open");
		if(open_path.isEmpty())
			return;

		QFile f(open_path);
		if(!f.open(QIODevice::ReadOnly))
			return;

		const QByteArray file_data= f.readAll();
		f.close();

		central_widget_.GetCSGTreeRoot()= DeserializeCSGExpressionTree(file_data);
	}

	void OnSave()
	{
		const QString save_path= QFileDialog::getSaveFileName(this, "Sazava - save");
		if(save_path.isEmpty())
			return;

		const QByteArray csg_tree_serialized= SerializeCSGExpressionTree(central_widget_.GetCSGTreeRoot());

		QFile f(save_path);
		if(!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
			return;

		f.write(csg_tree_serialized);
		f.close();
	}

private:
	CentralWidget central_widget_;
};

int Main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	MainWindow window;
	window.show();

	return app.exec();
}

} // namespace

} // namespace SZV

int main(int argc, char* argv[])
{
	return SZV::Main(argc, argv);
}
