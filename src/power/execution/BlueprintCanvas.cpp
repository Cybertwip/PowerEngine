#include "BlueprintCanvas.hpp"

#include "BlueprintNode.hpp"
#include "NodeProcessor.hpp"
#include "BlueprintNode.hpp"
#include "KeyPressNode.hpp"
#include "KeyReleaseNode.hpp"
#include "PrintNode.hpp"
#include "StringNode.hpp"
#include "ReflectedNode.hpp"


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
	
	assert(!SContextMenu); // Unique instance allowed;
	
	BlueprintCanvas::SContextMenu = std::make_unique<nanogui::Popup>(*this);

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

		if (!SContextMenu->contains(point, true, true)) {
			SContextMenu->set_visible(false);
		}
		
		if (down) {
			mSelectedNode = nullptr;
			
			// Query node again
			for (auto& node : mVisualNodes) {
				if (node->contains(point, true, true)){
					mSelectedNode = node.get();
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
				remove_child(*SContextMenu);
				add_child(*SContextMenu);
				setup_options();
				SContextMenu->set_position(nanogui::Vector2i(x + 32, y - 256));
				SContextMenu->set_visible(true);
				perform_layout(this->screen().nvg_context());
			}
		}
		
		mScrolling = false;
	});

	parent.register_motion_callback(GLFW_MOUSE_BUTTON_MIDDLE, [this](int width, int height, int x, int y, int dx, int dy, int button, bool down){
		
		mScrolling = down;
		
		if (mScrolling) {
			SContextMenu->set_visible(false);
		}
		
		// because dx and dy are global screen space and its range is -0.5 to 0.5
		float scaledDx = dx * (width / fixed_width()) * 2.0f;
		float scaledDy = dy * (height / fixed_height()) * 2.0f;
		
		mScrollX += scaledDx;
		mScrollY += scaledDy;
		
		mGrid->set_scroll_offset(nanogui::Vector2i(-mScrollX, -mScrollY));
		
		// Assuming mTestWidget's position is in screen space, update it
		
		for (auto& node : mVisualNodes) {
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

void BlueprintCanvas::on_output_pin_clicked(VisualPin& pin) {
	if (mActiveOutputPin.has_value()){
		mActiveOutputPin = std::nullopt;
	} else {
		mActiveOutputPin = pin;
	}
}

bool BlueprintCanvas::query_link(VisualPin& source_pin, VisualPin& destination_pin) {
	if (destination_pin.core_pin().links.empty()) {
		if (source_pin.get_type() == destination_pin.get_type() && &source_pin.node() != &destination_pin.node()) {
			return true;
		} else if (source_pin.get_type() != destination_pin.get_type()){
			return false;
		} else if (&source_pin.node() == &destination_pin.node()) {
			return false;
		}
	} else if (!destination_pin.core_pin().links.empty()) {
		return false;
	}
}

void BlueprintCanvas::on_input_pin_clicked(VisualPin& pin) {
	if (mActiveOutputPin && !mActiveInputPin && query_link(mActiveOutputPin->get(), pin)) {
		mActiveInputPin = pin;
		mNodeProcessor.create_link(*this, mNodeProcessor.get_next_id(), mActiveOutputPin->get(), mActiveInputPin->get());
		
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
		
		auto& node = mActiveOutputPin->get().node();
		
		NVGcolor color = node.color;
		
		if (mActiveOutputPin->get().get_type() == PinType::Flow) {
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
		
		for (auto& link : mLinks) {
			auto& start_pin = link->get_start();
			auto& end_pin = link->get_end();
			
			auto& node = start_pin.node();
			
			NVGcolor color = node.color;
			
			if (start_pin.get_type() == PinType::Flow) {
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
    SContextMenu->clear_children();
    mNodeOptions.clear();

    // Dynamically add an option for every reflected type
    auto all_types = power::reflection::ReflectionRegistry::get_all_types();
    for (const auto& type : all_types) {
        auto type_button = std::make_unique<nanogui::Button>(*SContextMenu, type.get_name());
        
        type_button->set_callback([this, type_name = type.get_name()](){
            SContextMenu->set_visible(false);

            // 1. Create the Core Node using the processor
            auto core_node_ptr = mNodeProcessor.create_node(type_name, UUIDGenerator::generate());
            if (!core_node_ptr) return;

            auto* reflected_core = dynamic_cast<ReflectedCoreNode*>(core_node_ptr.get());
            if (!reflected_core) return;
            
            // 2. Add it to the processor's list of managed nodes
            mNodeProcessor.add_node(std::move(core_node_ptr));
            
            // 3. Spawn the Visual Node on the canvas
            spawn_node<ReflectedVisualNode>(SContextMenu->position(), *reflected_core);
            perform_layout(this->screen().nvg_context());
        });
        mNodeOptions.push_back(std::move(type_button));
    }
    
    // You can still add hardcoded nodes if you want
    auto key_press_option = std::make_unique<nanogui::Button>(*SContextMenu, "KeyPress");
    key_press_option->set_callback([this](){
        SContextMenu->set_visible(false);
        auto& core = mNodeProcessor.add_node(mNodeProcessor.create_node("KeyPress", UUIDGenerator::generate()));
        spawn_node<KeyPressVisualNode>(SContextMenu->position(), static_cast<KeyPressCoreNode&>(core));
        perform_layout(this->screen().nvg_context());
    });
    mNodeOptions.push_back(std::move(key_press_option));

    SContextMenu->set_visible(false);
}


void BlueprintCanvas::clear() {
	clear_children();
	attach_popup();
	mSelectedNode = nullptr;
	mActiveInputPin = std::nullopt;
	mActiveOutputPin = std::nullopt;
	mVisualNodes.clear();
	mLinks.clear();
}

void BlueprintCanvas::attach_popup() {
	add_child(*SContextMenu);
}

void BlueprintCanvas::add_link(VisualPin& start, VisualPin& end) {
	mLinks.push_back(std::move(std::make_unique<VisualLink>(start, end)));
	mOnCanvasModifiedCallback();
}

void BlueprintCanvas::link(CorePin& start, CorePin& end) {
	
	auto* start_node = find_node(start.node.id);
	auto* end_node = find_node(end.node.id);
	
	assert(start_node && end_node);
	
	auto* start_pin = start_node->find_pin(start.id);
	auto* end_pin = end_node->find_pin(end.id);
	
	mLinks.push_back(std::move(std::make_unique<VisualLink>(*start_pin, *end_pin)));
	mOnCanvasModifiedCallback();
}

VisualBlueprintNode* BlueprintCanvas::find_node(UUID id) {
	
	auto node_it = std::find_if(mVisualNodes.begin(), mVisualNodes.end(), [id](auto& node) {
		return node->core_node().id == id;
	});
	
	if (node_it != mVisualNodes.end()) {
		return node_it->get();
	}
	
	return nullptr;
}

bool BlueprintCanvas::keyboard_event(int key, int scancode, int action, int modifiers) {
	// First, let the base canvas class handle its own events if needed.
	if (Canvas::keyboard_event(key, scancode, action, modifiers))
		return true;
	
	// Iterate through all visual nodes and forward the event to each one.
	// If a node handles the event (returns true), we can stop.
	for (auto& node : mVisualNodes) {
		if (node->keyboard_event(key, scancode, action, modifiers)) {
			return true;
		}
	}
	
	// No node handled the event.
	return false;
}


std::unique_ptr<nanogui::Popup> BlueprintCanvas::SContextMenu;
