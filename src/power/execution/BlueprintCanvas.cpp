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
, mScrollY(0) {
	
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
	
	// Register draw callback
	register_draw_callback([this]() {
		this->draw();
	});
}

void BlueprintCanvas::add_node(Node* node) {
	mNodes.push_back(node);
}

void BlueprintCanvas::draw() {
	render_pass().clear_color(0, background_color());
	mGrid->draw_content(nanogui::Matrix4f::identity(), mView, mProjection);
}

}
