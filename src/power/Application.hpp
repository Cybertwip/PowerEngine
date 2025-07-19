#pragma once

#include "animation/AnimationTimeProvider.hpp"
#include "serialization/SerializationModule.hpp"

#include <entt/entt.hpp>

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <GLFW/glfw3.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

class DraggableWindow : public nanogui::Window {
public:
	DraggableWindow(nanogui::Widget& parent, const std::string &title = "Drag Me")
	: nanogui::Window(parent, title) { // Corrected parent constructor call
		set_modal(false);
	}
	
	void set_drag_callback(std::function<void()> drag_callback) {
		m_drag_callback = drag_callback;
	}
	
	bool mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {
		// Custom behavior for dragging the window itself
		if (screen().drag_active()) {
			// This window is being controlled by the DraggableScreen's logic.
			// The screen will update our position. We just need to report success.
			// The actual set_position call is handled in DraggableScreen's cursor_pos_callback_event.
			// However, if logic were to change, the below is the correct way to handle it here.
			// set_position(m_pos + rel);
			return true;
		}
		
		return nanogui::Window::mouse_drag_event(p, rel, button, modifiers);
	}
	
	void do_drag_finish() {
		 m_children.clear(); 

		if (m_drag_callback) {
			m_drag_callback();
			m_drag_callback = nullptr;
		}
	}
	
private:
	std::function<void()> m_drag_callback;
};

namespace nanogui {

// NOTE: The existing DraggableScreen implementation has been preserved as requested.
// The primary fix is in how FileView interacts with it.
class DraggableScreen : public Screen {
public:
	DraggableScreen(const std::string &caption,
					bool fullscreen = false,
					bool depth_buffer = true,
					bool stencil_buffer = true,
					bool float_buffer = false,
					unsigned int gl_major = 3,
					unsigned int gl_minor = 2)
	: Screen(caption, fullscreen, depth_buffer, stencil_buffer, float_buffer, gl_major, gl_minor){ // Assuming a default size
	}
	
protected:
	virtual void cursor_pos_callback_event(double x, double y) override {
		Vector2i p((int)x, (int)y);
#if defined(_WIN32) || defined(__linux__) || defined(EMSCRIPTEN)
		p = Vector2i(Vector2f(p) / m_pixel_ratio);
#endif
		
		m_last_interaction = glfwGetTime();
		// If the draggable window has children, it means a drag is in progress.
		if (m_draggable_window->child_count() > 0) {
			p -= Vector2i(1, 2);
			bool ret = false;
			if (!m_drag_active) {
				// This logic block in the original code is tricky. A drag should be active
				// if we are in this state. Let's ensure it becomes active.
				m_drag_active = true;
				m_mouse_pos = p;
			}
			
			// The m_drag_widget is the m_draggable_window itself.
			ret = m_drag_widget->mouse_drag_event(p - m_drag_widget->parent()->get().absolute_position(), p - m_mouse_pos, m_mouse_state, m_modifiers);
			
			// Ensure the dragged widget (the draggable window) is moved.
			if(m_drag_active) {
				m_drag_widget->set_position(m_drag_widget->position() + (p - m_mouse_pos));
				ret = true;
			}
			
			if (!ret) {
				ret = mouse_motion_event(p, p - m_mouse_pos, m_mouse_state, m_modifiers);
			}
			
			m_mouse_pos = p;
			m_redraw |= ret;
			
		} else {
			Screen::cursor_pos_callback_event(x, y);
		}
	}
	
	virtual void mouse_button_callback_event(int button, int action, int modifiers) override {
		
		if (m_draggable_window->child_count() > 0) {
			m_modifiers = modifiers;
			m_last_interaction = glfwGetTime();
			
			if (action == GLFW_PRESS) {
				m_mouse_state |= 1 << button;
				// Set drag active only if a draggable item is being interacted with.
				// This is now handled by the widget initiating the drag.
				if (m_drag_widget) {
					m_drag_active = true;
				}
			} else if (action == GLFW_RELEASE) {
				m_mouse_state &= ~(1 << button);
				m_drag_active = false;
				if (m_drag_widget != nullptr) {
					// The drag is finished, execute the callback.
					m_draggable_window->do_drag_finish();
					m_drag_widget = nullptr;
				}
			}
			m_redraw |= mouse_button_event(m_mouse_pos, button, action == GLFW_PRESS, m_modifiers);
		} else {
			Screen::mouse_button_callback_event(button, action, modifiers);
		}
		
	}
	
	void initialize() { // Assuming this is called from your Application constructor
		m_draggable_window = std::make_shared<DraggableWindow>(*this, "");
		m_draggable_window->set_visible(false); // Start invisible
		set_drag_widget(nullptr, nullptr);
	}
	
	// This is the custom API to be used by widgets like FileView
	void set_drag_widget(Widget* widget, std::function<void()> drag_callback) override {
		if(widget == nullptr){
			m_draggable_window->set_visible(false);
			m_draggable_window->set_drag_callback(nullptr);
			m_drag_active = false;
			m_drag_widget = nullptr;
		} else {
			// This function's role is to "arm" the drag system.
			move_window_to_front(*m_draggable_window);
			m_draggable_window->set_visible(true);
			m_draggable_window->set_drag_callback(drag_callback);
			m_drag_widget = m_draggable_window.get();
			// Don't set m_drag_active here. It becomes active on mouse move.
		}
	}
	Widget* drag_widget() const override { return m_draggable_window.get(); }
	
private:
	std::shared_ptr<DraggableWindow> m_draggable_window;
};
} // namespace nanogui


class ICartridge;
class VirtualMachine;

class Actor;
class ActorManager;
class BlueprintManager;
class Canvas;
class CartridgeActorLoader;
class CameraManager;
class ExecutionManager;
class GizmoManager;
class MeshActor;
class MeshActorBuilder;
class MeshActorLoader;
class MeshBatch;
class RenderCommon;
class ShaderManager;
class ShaderWrapper;
class SimulationServer;
class SkinnedMeshBatch;
class SerializationModule;
class UiCommon;
class UiManager;

struct BatchUnit;

class Application : public nanogui::DraggableScreen
{
   public:
    Application();
	
	void initialize();

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;
	virtual void draw(NVGcontext *ctx) override;
	virtual void process_events() override;

	void register_click_callback(std::function<void(bool, int, int, int, int)> callback);
	
	void new_project_action();
	
	void new_scene_action();
	void save_scene_action();
	void load_scene_action();

   private:
	bool drop_event(Widget& sender, const std::vector<std::string> & filenames) override;

	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	
	AnimationTimeProvider mGlobalAnimationTimeProvider;
	std::unique_ptr<ActorManager> mActorManager;

	std::unique_ptr<ExecutionManager> mExecutionManager;
	std::unique_ptr<VirtualMachine> mVirtualMachine;
	
	std::optional<std::reference_wrapper<VirtualMachine>> mLoadedVirtualMachine;

	// Reversed unique_ptr declarations
	std::unique_ptr<ICartridge> mCartridge;
	std::unique_ptr<CartridgeActorLoader> mCartridgeActorLoader;
	std::unique_ptr<Canvas> mCanvas; // Place appropriately based on actual dependencies
	std::unique_ptr<UiManager> mUiManager;
	std::unique_ptr<GizmoManager> mGizmoManager;
	std::unique_ptr<MeshActorLoader> mGizmoActorLoader;
	std::unique_ptr<MeshActorLoader> mMeshActorLoader;
	std::unique_ptr<MeshActorBuilder> mMeshActorBuilder;
	std::unique_ptr<ShaderWrapper> mSkinnedShader;
	std::unique_ptr<ShaderWrapper> mMeshShader;
	std::unique_ptr<BatchUnit> mBatchUnit;
	std::unique_ptr<BatchUnit> mGizmoBatchUnit;
	std::unique_ptr<SkinnedMeshBatch> mSkinnedMeshBatch;
	std::unique_ptr<MeshBatch> mMeshBatch;
	std::unique_ptr<MeshBatch> mGizmoMeshBatch;
	std::unique_ptr<SkinnedMeshBatch> mGizmoSkinnedMeshBatch;
	std::shared_ptr<RenderCommon> mRenderCommon;
	std::shared_ptr<UiCommon> mUiCommon;
	std::unique_ptr<BlueprintManager> mBlueprintManager;
	std::unique_ptr<CameraManager> mCameraManager;
	std::unique_ptr<entt::registry> mEntityRegistry;
	std::unique_ptr<SimulationServer> mSimulationServer;
	std::unique_ptr<SerializationModule> mSerializationModule;

	std::queue<std::tuple<bool, int, int, int, int>> mClickQueue;
	std::vector<std::function<void(bool, int, int, int, int)>> mClickCallbacks;
	std::vector<std::function<void()>> mEventQueue;
};
