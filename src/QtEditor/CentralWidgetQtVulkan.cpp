#include "../Lib/CSGRenderer.hpp"
#include "../Lib/SelectionRenderer.hpp"
#include "CentralWidget.hpp"
#include "CSGNodesTreeWidget.hpp"
#include "NewNodeListWidget.hpp"
#include <QtGui/QKeyEvent>
#include <QtGui/QVulkanWindow>

namespace SZV
{

namespace
{

CSGTree::CSGTreeNode GetTestCSGTree()
{
	return CSGTree::AddChain
	{ {
		CSGTree::Box{ { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
	} };
}

struct SelectionBox
{
	m_Vec3 center;
	m_Vec3 size;
	m_Vec3 angles_deg;
};

class VulkanRenderer final : public QVulkanWindowRenderer, public I_WindowVulkan
{
public:
	explicit VulkanRenderer(
		QVulkanWindow& window,
		const CSGTree::CSGTreeNode& csg_tree_root,
		const SelectionBox& selection_box,
		const InputState& input_state)
		: window_(window)
		, csg_tree_root_(csg_tree_root)
		, selection_box_(selection_box)
		, input_state_(input_state)
		, camera_controller_(1.0f)
		, prev_tick_time_(Clock::now())
	{}

	void initResources() override
	{}

	void initSwapChainResources() override
	{
		csg_renderer_= nullptr;
		csg_renderer_= std::make_unique<CSGRenderer>(*this);

		selection_renderer_= nullptr;
		selection_renderer_= std::make_unique<SelectionRenderer>(*this);
	}

	void releaseSwapChainResources() override
	{
		csg_renderer_= nullptr;
		selection_renderer_ = nullptr;
	}

	void releaseResources() override
	{
		csg_renderer_= nullptr;
		selection_renderer_ = nullptr;
	}

	void startNextFrame() override
	{
		if(csg_renderer_ == nullptr || selection_renderer_ == nullptr)
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

		// TODO - draw real selection.
		selection_renderer_->EndFrame(
			command_buffer,
			camera_controller_,
			selection_box_.center, selection_box_.size, selection_box_.angles_deg);

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
	const SelectionBox& selection_box_;
	const InputState& input_state_;
	CameraController camera_controller_;
	std::unique_ptr<CSGRenderer> csg_renderer_;
	std::unique_ptr<SelectionRenderer> selection_renderer_;

	using Clock= std::chrono::steady_clock;
	Clock::time_point prev_tick_time_;
};

class VulkanWindow final : public QVulkanWindow
{
public:
	VulkanWindow(QWindow* const parent, const CSGTree::CSGTreeNode& csg_tree_root, const SelectionBox& selection_box)
		: QVulkanWindow(parent), csg_tree_root_(csg_tree_root), selection_box_(selection_box)
	{}

	QVulkanWindowRenderer *createRenderer() override
	{
		return new VulkanRenderer(*this, csg_tree_root_, selection_box_, input_state_);
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
	const SelectionBox& selection_box_;
};


class CentralWidgetQtVulkan final : public CentralWidgetBase
{
public:
	explicit CentralWidgetQtVulkan(QWidget* const parent)
		: CentralWidgetBase(parent)
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
		vulkan_window_= new VulkanWindow(this->windowHandle(), csg_tree_root_, selection_box_);
		vulkan_window_->setVulkanInstance(&vulkan_instance_);

		layout_.addWidget(&new_node_list_widget_);

		const auto down_layout= new QHBoxLayout();
		down_layout->addWidget(&csg_nodes_tree_widget_, 1);
		down_layout->addWidget(QWidget::createWindowContainer(vulkan_window_, this), 4);

		layout_.insertLayout(1, down_layout);

		setLayout(&layout_);

		connect(
			&csg_nodes_tree_widget_,
			&CSGNodesTreeWidget::selectionBoxChanged,
			this,
			[this](const m_Vec3& box_center, const m_Vec3& box_size, const m_Vec3& angles_deg)
			{
				selection_box_.center= box_center;
				selection_box_.size= box_size;
				selection_box_.angles_deg= angles_deg;
			} );
	}

public: // CentralWidgetBase
	CSGTree::CSGTreeNode& GetCSGTreeRoot() override
	{
		return csg_tree_model_.GetRoot();
	}

	const CSGTree::CSGTreeNode& GetCSGTreeRoot() const override
	{
		return csg_tree_model_.GetRoot();
	}

private:
	QVulkanInstance vulkan_instance_;
	VulkanWindow* vulkan_window_= nullptr;

	CSGTree::CSGTreeNode csg_tree_root_;
	SelectionBox selection_box_;
	CSGTreeModel csg_tree_model_;
	QVBoxLayout layout_;
	NewNodeListWidget new_node_list_widget_;
	CSGNodesTreeWidget csg_nodes_tree_widget_;
};

} // namespace

CentralWidgetBase* CreateCentralWidget(QWidget* const parent)
{
	return new CentralWidgetQtVulkan(parent);
}

} // namespace SZV
