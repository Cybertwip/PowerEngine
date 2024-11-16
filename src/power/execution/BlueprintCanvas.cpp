#include "BlueprintCanvas.hpp"

#include "Node.hpp"

#include "ShaderManager.hpp"

#include "graphics/drawing/Grid.hpp"
#include "ui/ScenePanel.hpp"

#include <nanogui/renderpass.h>

#include <GLFW/glfw3.h>

namespace blueprint {

BlueprintCanvas::BlueprintCanvas(ScenePanel& parent, nanogui::Screen& screen, nanogui::Color backgroundColor)
: Canvas(parent, screen, backgroundColor)
, mScrollX(0)
, mScrollY(0)
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

	// Register draw callback
	register_draw_callback([this]() {
		this->draw();
	});
	
}

void BlueprintCanvas::add_node(Node* node) {
	mNodes.push_back(node);
}

void BlueprintCanvas::on_output_pin_clicked(Pin& pin) {
	mActiveOutputPin = pin;
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
	
	if (mActiveOutputPin.has_value()) {
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
		nvgStrokeColor(ctx, nvgRGBA(255, 255, 0, 255)); // Yellow stroke
		nvgStrokeWidth(ctx, 3.0f);
		nvgStroke(ctx);
		
		// Draw circles at the start and end points
		float radius = 5.0f; // Radius of the circle
		
		// Circle at the start point
		nvgBeginPath(ctx);
		nvgCircle(ctx, pin_position.x(), pin_position.y(), radius);
		nvgFillColor(ctx, nvgRGBA(255, 255, 0, 255)); // Same color as the stroke
		nvgFill(ctx);
		
		// Circle at the end point
		nvgBeginPath(ctx);
		nvgCircle(ctx, ex, ey, radius);
		nvgFillColor(ctx, nvgRGBA(255, 255, 0, 255)); // Same color as the stroke
		nvgFill(ctx);
	}
}

}
