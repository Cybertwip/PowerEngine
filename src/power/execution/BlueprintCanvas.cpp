#include "BlueprintCanvas.hpp"

#include "BlueprintNode.hpp"
#include "NodeProcessor.hpp"
#include "BlueprintNode.hpp"
#include "KeyPressNode.hpp"
#include "KeyReleaseNode.hpp"
#include "PrintNode.hpp"
#include "StringNode.hpp"


#include "ShaderManager.hpp"

#include "graphics/drawing/Grid.hpp"
#include "ui/ScenePanel.hpp"

#include <nanogui/renderpass.h>
#include <nanogui/screen.h>

#include <nanovg.h>

#include <GLFW/glfw3.h>

BlueprintCanvas::BlueprintCanvas(ScenePanel& parent, nanogui::Screen& screen, NodeProcessor& nodeProcessor, std::function<void()> onCanvasModified, nanogui::Color backgroundColor)
: Canvas(parent, screen, backgroundColor)
, mScrollX(0)
, mScrollY(0)
, mScrolling(false)
, mNodeProcessor(nodeProcessor)
, mOnCanvasModifiedCallback(onCanvasModified)
, mSelectedNode(nullptr)
, mMousePosition(0, 0) {
	mShaderManager = std::make_unique<ShaderManager>(*this);
	
	mGrid = std::make_unique<Grid2d>(*mShaderManager);
	mContextMenu = std::make_unique<nanogui::Popup>(*this);

	// Adjusted orthographic projection parameters
	float left = -1.0f;
	float right = 1.0f;
	float bottom = -1.0f;
	float top = 1.0f;
	float near = -1.0f;
	float far = 1.0f;
	
	mProjection = nanogui::Matrix4f::ortho(left, right, bottom, top, near, far);
	
	mView = nanogui::Matrix4f::look_at(
									   nanogui::Vector3f(0, 0, 1),  // Camera position
									   nanogui::Vector3f(0, 0, 0),  // Look-at point
									   nanogui::Vector3f(0, 1, 0)   // Up direction
									   );
	
	parent.register_click_callback(GLFW_MOUSE_BUTTON_LEFT, [this](bool down, int width, int height, int x, int y) {
		auto point = nanogui::Vector2i(x, y) + position();

		if (!mContextMenu->contains(point, true, true)) {
			mContextMenu->set_visible(false);
		}
		
		if (down) {
			mSelectedNode = nullptr;
			
			// Query node again
			for (auto* node : mNodes) {
				if (node->contains(point, true, true)){
					mSelectedNode = node;
					break;
				}
			}
		} else {
			mActiveOutputPin = std::nullopt;
			mActiveInputPin = std::nullopt;
		}
		
	});
	
	parent.register_click_callback(GLFW_MOUSE_BUTTON_RIGHT, [this](bool down, int width, int height, int x, int y) {
		
		if (!down) {
			if (!mScrolling) {
				remove_child(*mContextMenu);
				add_child(*mContextMenu);
				setup_options();
				mContextMenu->set_position(nanogui::Vector2i(x + 32, y - 256));
				mContextMenu->set_visible(true);
				perform_layout(this->screen().nvg_context());
			}
		}
		
		mScrolling = false;
	});

	parent.register_motion_callback(GLFW_MOUSE_BUTTON_MIDDLE, [this](int width, int height, int x, int y, int dx, int dy, int button, bool down){
		
		mScrolling = down;
		
		if (mScrolling) {
			mContextMenu->set_visible(false);
		}
		
		// because dx and dy are global screen space and its range is -0.5 to 0.5
		float scaledDx = dx * (width / fixed_width()) * 2.0f;
		float scaledDy = dy * (height / fixed_height()) * 2.0f;
		
		mScrollX += scaledDx;
		mScrollY += scaledDy;
		
		mGrid->set_scroll_offset(nanogui::Vector2i(-mScrollX, -mScrollY));
		
		// Assuming mTestWidget's position is in screen space, update it
		
		for (auto* node : mNodes) {
			nanogui::Vector2i currentPos = node->position();
			node->set_position(currentPos + nanogui::Vector2i(dx, dy));
		}
	});
	
	mHeaderHeight = theme().m_window_header_height;
	
	// Register draw callback
	register_draw_callback([this]() {
		this->draw();
	});
	
}

void BlueprintCanvas::add_node(BlueprintNode* node) {
	mNodes.push_back(node);
	mOnCanvasModifiedCallback();
}

void BlueprintCanvas::on_output_pin_clicked(Pin& pin) {
	if (mActiveOutputPin.has_value()){
		mActiveOutputPin = std::nullopt;
	} else {
		mActiveOutputPin = pin;
	}
}

bool BlueprintCanvas::query_link(Pin& source_pin, Pin& destination_pin) {
	if (destination_pin.links.empty()) {
		if (source_pin.type == destination_pin.type && source_pin.node != destination_pin.node) {
			return true;
		} else if (source_pin.type != destination_pin.type){
			return false;
		} else if (source_pin.node == destination_pin.node) {
			return false;
		}
	} else if (!destination_pin.links.empty()) {
		return false;
	}
}

void BlueprintCanvas::on_input_pin_clicked(Pin& pin) {
	if (mActiveOutputPin && !mActiveInputPin && query_link(mActiveOutputPin->get(), pin)) {
		mActiveInputPin = pin;
		mNodeProcessor.create_link(*this, mNodeProcessor.get_next_id(), *mActiveOutputPin, pin);
		mActiveOutputPin->get().links.push_back(mLinks.back());
		pin.links.push_back(mLinks.back());
		
		if(mActiveOutputPin->get().node->link){
			mActiveOutputPin->get().node->link();
		}

		if(pin.node->link){
			pin.node->link();
		}
		
		mActiveOutputPin = std::nullopt;
		mActiveInputPin = std::nullopt;
	} else {
		mActiveOutputPin = std::nullopt;
		mActiveInputPin = std::nullopt;
	}
}

bool BlueprintCanvas::mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	return nanogui::Widget::mouse_button_event(p, button, down, modifiers); // don't delegate to Canvas, propagate via Widget
}

bool BlueprintCanvas::mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {
	mMousePosition = nanogui::Vector2i(p.x() + absolute_position().x(), p.y() + absolute_position().y() - mHeaderHeight);
	return nanogui::Widget::mouse_motion_event(p, rel, button, modifiers); // don't delegate to Canvas, propagate via Widget
}

void BlueprintCanvas::draw() {
	//	render_pass().clear_color(0, background_color()); // globally cleared
	render_pass().clear_depth(1.0f);
	
	render_pass().set_depth_test(nanogui::RenderPass::DepthTest::Always, true); // draw on top
	mGrid->draw_content(nanogui::Matrix4f::identity(), mView, mProjection);
}

void BlueprintCanvas::draw(NVGcontext *ctx) {
	Canvas::draw(ctx);
	
	if (mActiveOutputPin.has_value() && !mActiveInputPin.has_value()) {
		
		auto* node = mActiveOutputPin->get().node;
		
		NVGcolor color = node->color;
		
		if (mActiveOutputPin->get().type == PinType::Flow) {
			color = nvgRGBA(255, 255, 255, 255);
		}
		
		// Get canvas and pin positions
		nanogui::Vector2i canvas_position = absolute_position();
		nanogui::Vector2i pin_position = mActiveOutputPin->get().absolute_position();
		nanogui::Vector2i pin_size = mActiveOutputPin->get().fixed_size();
		
		// Translate the context to account for the canvas position
		nvgTranslate(ctx, m_pos.x(), m_pos.y());
		nvgTranslate(ctx, -canvas_position.x(), -canvas_position.y());
		
		// Define Bézier curve
		nvgBeginPath(ctx);
		nvgMoveTo(ctx, pin_position.x() + pin_size.x() * 0.5f, pin_position.y() + pin_size.y() * 0.5f);
		
		// Set control points and end point (example values for demonstration)
		// Define control points and endpoint for the Bézier curve.
		float c1x = pin_position.x() + 50; // Example control point 1 x
		float c1y = pin_position.y() - 30; // Example control point 1 y
		float c2x = mMousePosition.x() - 50; // Example control point 2 x
		float c2y = mMousePosition.y() + 30; // Example control point 2 y
		float ex = mMousePosition.x(); // Endpoint x
		float ey = mMousePosition.y(); // Endpoint y
		
		// Draw the Bézier curve.
		nvgBezierTo(ctx, c1x, c1y, c2x, c2y, ex, ey);
		nvgStrokeColor(ctx, color);
		nvgStrokeWidth(ctx, 3.0f);
		nvgStroke(ctx);
		
		// Draw circles at the start and end points
		float radius = 7.0f; // Radius of the circle
		
		// Circle at the start point
		nvgBeginPath(ctx);
		nvgCircle(ctx, pin_position.x() + pin_size.x() * 0.5f, pin_position.y() + pin_size.y() * 0.5f, radius);
		nvgFillColor(ctx, color); // Same color as the stroke
		nvgFill(ctx);
		
		// Circle at the end point
		nvgBeginPath(ctx);
		nvgCircle(ctx, ex, ey, radius);
		nvgFillColor(ctx, color); // Same color as the stroke
		nvgFill(ctx);
		
		nvgTranslate(ctx, canvas_position.x(), canvas_position.y());
		nvgTranslate(ctx, -m_pos.x(), -m_pos.y());
	}
	
	if (!mLinks.empty()) {
		// Get canvas and pin positions
		nanogui::Vector2i canvas_position = absolute_position();
		// Translate the context to account for the canvas position
		nvgTranslate(ctx, m_pos.x(), m_pos.y());
		nvgTranslate(ctx, -canvas_position.x(), -canvas_position.y());
		
		
		for (auto* link : mLinks) {
			auto& start_pin = link->get_start();
			auto& end_pin = link->get_end();
			
			auto* node = start_pin.node;
			
			NVGcolor color = node->color;
			
			if (start_pin.type == PinType::Flow) {
				color = nvgRGBA(255, 255, 255, 255);
			}
			
			
			nanogui::Vector2i start_pin_position = start_pin.absolute_position();
			nanogui::Vector2i start_pin_size = start_pin.fixed_size();
			
			nanogui::Vector2i end_pin_position = end_pin.absolute_position();
			nanogui::Vector2i end_pin_size = end_pin.fixed_size();
			
			
			// Define Bézier curve
			nvgBeginPath(ctx);
			nvgMoveTo(ctx, start_pin_position.x() + start_pin_size.x() * 0.5f, start_pin_position.y() + start_pin_size.y() * 0.5f);
			
			// Set control points and end point (example values for demonstration)
			// Define control points and endpoint for the Bézier curve.
			float c1x = start_pin_position.x() + 50; // Example control point 1 x
			float c1y = start_pin_position.y() - 30; // Example control point 1 y
			float c2x = end_pin_position.x() - 50; // Example control point 2 x
			float c2y = end_pin_position.y() + 30; // Example control point 2 y
			float ex = end_pin_position.x() + end_pin_size.x() * 0.5f; // Endpoint x
			float ey = end_pin_position.y() + end_pin_size.y() * 0.5f; // Endpoint y
			
			// Draw the Bézier curve.
			nvgBezierTo(ctx, c1x, c1y, c2x, c2y, ex, ey);
			nvgStrokeColor(ctx, color);
			nvgStrokeWidth(ctx, 3.0f);
			nvgStroke(ctx);
			
			// Draw circles at the start and end points
			float radius = 7.0f; // Radius of the circle
			
			// Circle at the start point
			nvgBeginPath(ctx);
			nvgCircle(ctx, start_pin_position.x() + start_pin_size.x() * 0.5f, start_pin_position.y() + start_pin_size.y() * 0.5f, radius);
			nvgFillColor(ctx, color); // Same color as the stroke
			nvgFill(ctx);
			
			// Circle at the end point
			nvgBeginPath(ctx);
			nvgCircle(ctx, ex, ey, radius);
			nvgFillColor(ctx, color); // Same color as the stroke
			nvgFill(ctx);
		}
		
		nvgTranslate(ctx, canvas_position.x(), canvas_position.y());
		nvgTranslate(ctx, -m_pos.x(), -m_pos.y());
	}

	if (mSelectedNode) {
		nanogui::Vector2i node_pos = mSelectedNode->absolute_position();
		nanogui::Vector2i node_size = mSelectedNode->size();
		
		// Get canvas position for proper translation
		nanogui::Vector2i canvas_position = absolute_position();
		
		// Translate to correct position
		nvgTranslate(ctx, m_pos.x(), m_pos.y());
		nvgTranslate(ctx, -canvas_position.x(), -canvas_position.y());
		
		// Begin drawing the contour
		nvgBeginPath(ctx);
		
		// Draw rectangle around the node
		nvgRect(ctx,
				node_pos.x() - 4,  // Slightly larger than the node
				node_pos.y() - 4,
				node_size.x() + 8, // Add padding on both sides
				node_size.y() + 8
				);
		
		// Set contour style
		nvgStrokeColor(ctx, nvgRGBA(255, 255, 255, 100)); // Semi-transparent white
		nvgStrokeWidth(ctx, 2.0f);
		nvgStroke(ctx);
		
		// Reset translation
		nvgTranslate(ctx, canvas_position.x(), canvas_position.y());
		nvgTranslate(ctx, -m_pos.x(), -m_pos.y());
	}
}

void BlueprintCanvas::setup_options() {
	mContextMenu->shed_children();
	
	auto key_press_option = std::make_unique<nanogui::Button>(*mContextMenu, "Key Press");
	auto key_release_option = std::make_unique<nanogui::Button>(*mContextMenu, "Key Release");
	auto string_option = std::make_unique<nanogui::Button>(*mContextMenu, "String");
	auto print_option = std::make_unique<nanogui::Button>(*mContextMenu, "Print");
	
	key_press_option->set_callback([this](){
		mContextMenu->set_visible(false);
		add_node(mNodeProcessor.spawn_node<KeyPressNode>(*this, mContextMenu->position()));
		perform_layout(this->screen().nvg_context());
	});
	
	key_release_option->set_callback([this](){
		mContextMenu->set_visible(false);
		add_node(mNodeProcessor.spawn_node<KeyReleaseNode>(*this, mContextMenu->position()));
		perform_layout(this->screen().nvg_context());
	});
	
	string_option->set_callback([this](){
		mContextMenu->set_visible(false);
		add_node(mNodeProcessor.spawn_node<StringNode>(*this, mContextMenu->position()));
		perform_layout(this->screen().nvg_context());
	});
	
	print_option->set_callback([this](){
		mContextMenu->set_visible(false);
		add_node(mNodeProcessor.spawn_node<PrintNode>(*this, mContextMenu->position()));
		perform_layout(this->screen().nvg_context());
	});
	
	mNodeOptions.clear();
	
	mNodeOptions.push_back(std::move(key_press_option));
	mNodeOptions.push_back(std::move(key_release_option));
	mNodeOptions.push_back(std::move(string_option));
	mNodeOptions.push_back(std::move(print_option));
	
	mContextMenu->set_visible(false);
}

void BlueprintCanvas::clear() {
	shed_children();
	mActiveInputPin = std::nullopt;
	mActiveOutputPin = std::nullopt;
	mNodes.clear();
	mLinks.clear();
	mSelectedNode = nullptr;
	mContextMenu = std::make_unique<nanogui::Popup>(*this);
}

void BlueprintCanvas::add_link(Link* link) {
	mLinks.push_back(link);
	mOnCanvasModifiedCallback();
}
