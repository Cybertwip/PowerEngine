#include "BlueprintCanvas.hpp"

#include "BlueprintNode.hpp"
#include "NodeProcessor.hpp"

#include "ShaderManager.hpp"

#include "graphics/drawing/Grid.hpp"
#include "ui/ScenePanel.hpp"

#include <nanogui/renderpass.h>

#include <nanovg.h>

#include <GLFW/glfw3.h>

namespace blueprint {

BlueprintCanvas::BlueprintCanvas(ScenePanel& parent, nanogui::Screen& screen, NodeProcessor& nodeProcessor, nanogui::Color backgroundColor)
: Canvas(parent, screen, backgroundColor)
, mScrollX(0)
, mScrollY(0)
, mNodeProcessor(nodeProcessor)
, mMousePosition(0, 0) {
	mShaderManager = std::make_unique<ShaderManager>(*this);
	
	mGrid = std::make_unique<Grid2d>(*mShaderManager);
	
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
	
	parent.register_motion_callback(GLFW_MOUSE_BUTTON_MIDDLE, [this](int width, int height, int x, int y, int dx, int dy, int button, bool down){
		
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
	
	add_node(mNodeProcessor.spawn_key_release_node(*this, nanogui::Vector2i(fixed_size().x() / 8, fixed_size().y() / 8)));
	
	add_node(mNodeProcessor.spawn_string_node(*this, nanogui::Vector2i(fixed_size().x() / 4, fixed_size().y() / 4)));
	
	add_node(mNodeProcessor.spawn_print_string_node(*this, nanogui::Vector2i(fixed_size().x() / 2, fixed_size().y() / 2)));
	
	// Register draw callback
	register_draw_callback([this]() {
		this->draw();
	});
	
}

void BlueprintCanvas::add_node(BlueprintNode* node) {
	mNodes.push_back(node);
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
	if (mActiveOutputPin.has_value() && query_link(mActiveOutputPin->get(), pin)) {
		mActiveInputPin = pin;
		mLinks.push_back(mNodeProcessor.create_link(*this, *mActiveOutputPin, pin));
		mActiveOutputPin->get().links.push_back(mLinks.back());
		pin.links.push_back(mLinks.back());
		
		
		//		if(mActiveOutputPin->get().node->link){
		//			mActiveOutputPin->get().node->link();
		//		}
		//
		//		if(pin.node->link){
		//			pin.node->link();
		//		}
		
		mActiveOutputPin = std::nullopt;
		mActiveInputPin = std::nullopt;
	} else {
		mActiveOutputPin = std::nullopt;
		mActiveInputPin = std::nullopt;
	}
}

bool BlueprintCanvas::mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {
	mMousePosition = nanogui::Vector2i(p.x() + absolute_position().x(), p.y() + absolute_position().y() - mHeaderHeight);
}

void BlueprintCanvas::draw() {
	render_pass().clear_color(0, background_color());
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
	}
}

void BlueprintCanvas::process_events() {
	mNodeProcessor.evaluate();
}
