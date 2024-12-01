#include "BlueprintNode.hpp"

#include "BlueprintCanvas.hpp"

void CoreNode::build() {
	for (auto& input : inputs) {
		input->kind = PinKind::Input;
	}
	for (auto& output : outputs) {
		output->kind = PinKind::Output;
	}
}

void CoreNode::reset_flow() {
	for (auto& input : inputs) {
		if (input->type == PinType::Flow) {
			input->can_flow = false;
		}
	}
	for (auto& output : outputs) {
		if (output->type == PinType::Flow) {
			output->can_flow = false;
		}
	}
}

long long CoreNode::get_next_id() {
	return next_id++;
}

VisualPin::VisualPin(nanogui::Widget& parent, CorePin& core_pin)
: nanogui::ToolButton(parent, FA_CIRCLE_NOTCH, "")
, mCorePin(core_pin) {
	
}

VisualBlueprintNode::VisualBlueprintNode(BlueprintCanvas& parent, const std::string& name, nanogui::Vector2i position, nanogui::Vector2i size, CoreNode& coreNode)
: nanogui::Window(parent, name)
, mCanvas(parent)
, mCoreNode(coreNode) {
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
	
	// Left column inputs placeholder
	mLeftColumn = std::make_unique<nanogui::Widget>(*mColumnContainer);
	mLeftColumn->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Minimum, 0, 0));
	
	// Middle column for the node data
	mDataColumn = std::make_unique<nanogui::Widget>(*mColumnContainer);
	mDataColumn->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Minimum, 0, 0));
	
	// Right column for output pins: Aligned to the right edge
	mRightColumn = std::make_unique<nanogui::Widget>(*mColumnContainer);
	mRightColumn->set_layout(std::make_unique<nanogui::BoxLayout>(nanogui::Orientation::Vertical, nanogui::Alignment::Minimum, 0, 0));
	
	mRightColumn->set_position(nanogui::Vector2i(fixed_size().x() - 48, 0));
	
	create_visual_pins();

}

void VisualBlueprintNode::create_visual_pins() {
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

void VisualBlueprintNode::perform_layout(NVGcontext *ctx) {
	Window::perform_layout(ctx);

	int hh = theme().m_window_header_height;
	
	mFlowContainer->set_position(nanogui::Vector2i(0, 0));
	mColumnContainer->set_position(nanogui::Vector2i(5, hh + 3));
}

void VisualBlueprintNode::draw(NVGcontext *ctx) {
	int ds = theme().m_window_drop_shadow_size, cr = theme().m_window_corner_radius;
	int hh = theme().m_window_header_height;
	
	/* Draw window */
	nvgSave(ctx);
	nvgBeginPath(ctx);
	nvgRoundedRect(ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), cr);
	
	nvgFillColor(ctx, m_mouse_focus ? theme().m_window_fill_focused
				 : theme().m_window_fill_unfocused);
	nvgFill(ctx);
	
	
	/* Draw a drop shadow */
	NVGpaint shadow_paint = nvgBoxGradient(
										   ctx, m_pos.x(), m_pos.y(), m_size.x(), m_size.y(), cr*2, ds*2,
										   theme().m_drop_shadow, theme().m_transparent);
	
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
		/* Draw header */
		NVGpaint header_paint = nvgLinearGradient(
												  ctx, m_pos.x(), m_pos.y(), m_pos.x(),
												  m_pos.y() + hh,
												  theme().m_window_header_gradient_top,
												  theme().m_window_header_gradient_bot);
		
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
		nvgText(ctx, m_pos.x() + m_size.x() / 2,
				m_pos.y() + hh / 2, m_title.c_str(), nullptr);
		
		nvgFontBlur(ctx, 0);
		nvgFillColor(ctx, theme().m_window_title_focused);
		nvgText(ctx, m_pos.x() + m_size.x() / 2, m_pos.y() + hh / 2 - 1,
				m_title.c_str(), nullptr);
	}
	
	nvgRestore(ctx);
	
	nvgTranslate(ctx, m_pos.x(), m_pos.y());
	
	mFlowContainer->draw(ctx);
	mColumnContainer->draw(ctx);
}
