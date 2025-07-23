#include "BlueprintNode.hpp"

#include "BlueprintCanvas.hpp"

#include "serialization/UUID.hpp"

inline void CoreNode::raise_event() {
	// Step 1: PULL data from connected nodes into this node's input pins.
	for (const auto& input_pin : inputs) {
		// We only care about connected data pins.
		if (input_pin->type != PinType::Flow && !input_pin->links.empty()) {
			// Assuming one connection per data input pin for simplicity
			CoreNode& source_node = input_pin->links[0]->get_start().node;
			// Use dynamic_cast to check if the source node is a data node
			if (auto* data_node = dynamic_cast<DataCoreNode*>(&source_node)) {
				input_pin->set_data(data_node->get_data());
			}
		}
	}
	
	// Step 2: EVALUATE this node's logic.
	if (this->evaluate()) {
		// Step 3: PUSH this node's output data to connected nodes.
		for (const auto& output_pin : outputs) {
			if (output_pin->type != PinType::Flow) {
				auto data_to_push = output_pin->get_data();
				// Transfer data to the input pin of every connected node.
				for (Link* link : output_pin->links) {
					link->get_end().set_data(data_to_push);
				}
			}
		}
		// Step 4: Propagate the execution flow.
		for (const auto& output_pin : outputs) {
			if (output_pin->type == PinType::Flow) {
				for (Link* link : output_pin->links) {
					link->get_end().node.raise_event();
				}
				// A node can only have one output flow pin.
				break;
			}
		}
	}
}

inline void CoreNode::build() {
	for (auto& input : inputs) {
		input->kind = PinKind::Input;
	}
	for (auto& output : outputs) {
		output->kind = PinKind::Output;
	}
}

inline UUID CoreNode::get_next_id() {
	return UUIDGenerator::generate();
}

inline VisualPin::VisualPin(nanogui::Widget& parent, CorePin& core_pin)
: nanogui::ToolButton(parent, FA_CIRCLE_NOTCH, ""), mCorePin(core_pin) {
}

inline VisualBlueprintNode::VisualBlueprintNode(BlueprintCanvas& parent, const std::string& name, nanogui::Vector2i position, nanogui::Vector2i size, CoreNode& coreNode)
: nanogui::Window(parent, name), mCanvas(parent), mCoreNode(coreNode) {
	set_draggable(true);
	set_fixed_size(size);
	set_layout(std::make_unique<nanogui::GroupLayout>(5, 0));
	set_position(position);
	mCoreNode.set_position(position);
	
	int hh = theme().m_window_header_height;
	mFlowContainer = std::make_unique<PassThroughWidget>(*this);
	mFlowContainer->set_fixed_size(nanogui::Vector2i(size.x(), hh));
	
	mColumnContainer = std::make_unique<PassThroughWidget>(*this);
	mColumnContainer->set_fixed_size(nanogui::Vector2i(size.x(), size.y() - hh));
	mColumnContainer->set_layout(std::make_unique<nanogui::GridLayout>(nanogui::Orientation::Horizontal, 3, nanogui::Alignment::Fill));
	
	mLeftColumn = std::make_unique<nanogui::Widget>(*mColumnContainer);
	mLeftColumn->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Minimum, 0, 0));
	
	mDataColumn = std::make_unique<nanogui::Widget>(*mColumnContainer);
	mDataColumn->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Minimum, 0, 0));
	
	mRightColumn = std::make_unique<nanogui::Widget>(*mColumnContainer.get());
	mRightColumn->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Minimum, 0, 0));
	mRightColumn->set_position(nanogui::Vector2i(fixed_size().x() - 48, 0));
	
	create_visual_pins();
}

inline void VisualBlueprintNode::create_visual_pins() {
	for (const auto& core_pin : mCoreNode.get_inputs()) {
		auto visual_pin = std::make_unique<VisualPin>(core_pin->type == PinType::Flow ? *mFlowContainer : *mLeftColumn, *core_pin);
		visual_pin->set_fixed_size(nanogui::Vector2i(22, 22));
		
		if (core_pin->type == PinType::Flow) {
			visual_pin->set_icon(FA_PLAY);
			visual_pin->set_position(nanogui::Vector2i(5, 3));
		}
		auto& visual_pin_ref = *visual_pin;
		visual_pin->set_hover_callback([this, &visual_pin_ref](){
			nanogui::async([this, &visual_pin_ref](){
				mCanvas.on_input_pin_clicked(visual_pin_ref);
			});
		});
		
		mVisualPins.push_back(std::move(visual_pin));
	}
	for (const auto& core_pin : mCoreNode.get_outputs()) {
		auto visual_pin = std::make_unique<VisualPin>(core_pin->type == PinType::Flow ? *mFlowContainer : *mRightColumn, *core_pin);
		visual_pin->set_fixed_size(nanogui::Vector2i(22, 22));
		
		if (core_pin->type == PinType::Flow) {
			visual_pin->set_icon(FA_PLAY);
			visual_pin->set_position(nanogui::Vector2i(fixed_size().x() - 22 - 5, 3));
		}
		
		auto& visual_pin_ref = *visual_pin;
		visual_pin->set_click_callback([this, &visual_pin_ref](){
			nanogui::async([this, &visual_pin_ref](){
				mCanvas.on_output_pin_clicked(visual_pin_ref);
			});
		});
		
		mVisualPins.push_back(std::move(visual_pin));
	}
}

inline void VisualBlueprintNode::perform_layout(NVGcontext *ctx) {
	Window::perform_layout(ctx);
	int hh = theme().m_window_header_height;
	mFlowContainer->set_position(nanogui::Vector2i(0, 0));
	mColumnContainer->set_position(nanogui::Vector2i(5, hh + 3));
}

inline void VisualBlueprintNode::draw(NVGcontext *ctx) {
	// Standard nanogui window drawing logic
	int ds = theme().m_window_drop_shadow_size, cr = theme().m_window_corner_radius;
	int hh = theme().m_window_header_height;
	
	nvgSave(ctx);
	nvgBeginPath(ctx);
	nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), cr);
	nvgFillColor(ctx, m_mouse_focus ? theme().m_window_fill_focused : theme().m_window_fill_unfocused);
	nvgFill(ctx);
	
	NVGpaint shadow_paint = nvgBoxGradient(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), cr*2, ds*2, theme().m_drop_shadow, theme().m_transparent);
	nvgSave(ctx);
	nvgResetScissor(ctx);
	nvgBeginPath(ctx);
	nvgRect(ctx, m_pos.x()-ds,m_pos.y()-ds, m_size.x()+2*ds, m_size.y()+2*ds);
	nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), cr);
	nvgPathWinding(ctx, NVG_HOLE);
	nvgFillPaint(ctx, shadow_paint);
	nvgFill(ctx);
	nvgRestore(ctx);
	
	if (!m_title.empty()) {
		NVGpaint header_paint = nvgLinearGradient(ctx, m_pos.x(), m_pos.y(), m_pos.x(), m_pos.y() + hh, theme().m_window_header_gradient_top, theme().m_window_header_gradient_bot);
		nvgBeginPath(ctx);
		nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), hh, cr);
		nvgFillPaint(ctx, header_paint);
		nvgFill(ctx);
		nvgBeginPath(ctx);
		nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), hh, cr);
		nvgStrokeColor(ctx, theme().m_window_header_sep_top);
		nvgSave(ctx);
		nvgIntersectScissor(ctx, m_pos.x(), m_pos.y(), m_size.x(), 0.5f);
		nvgStroke(ctx);
		nvgRestore(ctx);
		nvgBeginPath(ctx);
		nvgMoveTo(ctx, m_pos.x() + 0.5f, m_pos.y() + hh - 1.5f);
		nvgLineTo(ctx, m_pos.x() + m_size.x() - 0.5f, m_pos.y() + hh - 1.5);
		nvgStrokeColor(ctx, theme().m_window_header_sep_bot);
		nvgStroke(ctx);
		nvgFontSize(ctx, 18.0f);
		nvgFontFace(ctx, "sans-bold");
		nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFontBlur(ctx, 2);
		nvgFillColor(ctx, theme().m_drop_shadow);
		nvgText(ctx, m_pos.x() + m_size.x() / 2, m_pos.y() + hh / 2, m_title.c_str(), nullptr);
		nvgFontBlur(ctx, 0);
		nvgFillColor(ctx, theme().m_window_title_focused);
		nvgText(ctx, m_pos.x() + m_size.x() / 2, m_pos.y() + hh / 2 - 1, m_title.c_str(), nullptr);
	}
	nvgRestore(ctx);
	
	// Draw the child widgets (pins, etc.)
	nvgTranslate(ctx, m_pos.x(), m_pos.y());
	mFlowContainer->draw(ctx);
	mColumnContainer->draw(ctx);
}
