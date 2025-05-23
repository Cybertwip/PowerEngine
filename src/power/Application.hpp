#pragma once

#include "animation/AnimationTimeProvider.hpp"

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
	DraggableWindow(nanogui::Widget& parent, const std::string &title = "Drag Me") : nanogui::Window(parent) {
		set_modal(false);   // We want it to be freely interactive
		
		set_layout(std::make_unique< nanogui::BoxLayout>(nanogui::Orientation::Horizontal,
													   nanogui::Alignment::Fill, 10, 10));

	}
	
	void set_drag_callback(std::function<void()> drag_callback) {
		m_drag_callback = drag_callback;
	}
		
	bool mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {
		// Custom behavior for dragging the window itself
		if (screen().drag_active()) {
			set_visible(true);
			set_position(m_pos + rel);
			return true;
		} else {
			set_visible(false);
		}
		
		return false;
	}
	
	void do_drag_finish() {
		m_children.clear(); // hollow clearing, deallocation is own responsibility

		if (m_drag_callback) {
			m_drag_callback();
			m_drag_callback = nullptr;
		}
		
	}
	
private:
	std::function<void()> m_drag_callback;
};

namespace nanogui {

class DraggableScreen : public Screen {
public:
	DraggableScreen(const std::string &caption,
					bool fullscreen = false,
					bool depth_buffer = true,
					bool stencil_buffer = true,
					bool float_buffer = false,
					unsigned int gl_major = 3,
					unsigned int gl_minor = 2)
	: Screen(caption, fullscreen, depth_buffer, stencil_buffer, float_buffer, gl_major, gl_minor){
	}
	
protected:
	virtual void cursor_pos_callback_event(double x, double y) override {
		Vector2i p((int)x, (int)y);
#if defined(_WIN32) || defined(__linux__) || defined(EMSCRIPTEN)
		p = Vector2i(Vector2f(p) / m_pixel_ratio);
#endif

		m_last_interaction = glfwGetTime();
		if (m_draggable_window->children().size() != 0) {
			p -= Vector2i(1, 2);
			bool ret = false;
			if (!m_drag_active) {
				if (m_drag_widget != nullptr) {
					if (m_drag_widget->contains(p)){
						// Move the drag widget to the front of the hierarchy to ensure it is topmost
						move_widget_to_top(*m_drag_widget);
						m_drag_active = true;
						m_mouse_pos = p;

					}
				}
			} else {
				ret = m_drag_widget->mouse_drag_event(p - m_drag_widget->parent()->get().absolute_position(), p - m_mouse_pos, m_mouse_state, m_modifiers);
				// Ensure the dragged widget stays on top during the drag
				
				if (ret) {
					move_widget_to_top(*m_drag_widget);
				}
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
		
		if (m_draggable_window->children().size() != 0) {
			m_modifiers = modifiers;
			m_last_interaction = glfwGetTime();
			if (m_focused_widget) {
				auto window =
				dynamic_cast<Window*>(&m_focused_widget->get());;
				if (window && window->modal()) {
					if (!window->contains(m_mouse_pos))
						return;
				}
			}
			
			if (action == GLFW_PRESS) {
				m_mouse_state |= 1 << button;
				if (m_drag_widget) {
					m_drag_active = true;
				}
			} else if (action == GLFW_RELEASE) {
				m_mouse_state &= ~(1 << button);
				m_drag_active = false;
				if (m_drag_widget != nullptr) {
					if (m_drag_widget->visible()) {
						m_drag_widget->set_visible(false);
						
						m_draggable_window->do_drag_finish();
					}
					
					m_drag_widget = nullptr;
				}
				
			}
			
			m_redraw |= mouse_button_event(m_mouse_pos, button, action == GLFW_PRESS, m_modifiers);
		} else {
			Screen::mouse_button_callback_event(button, action, modifiers);
		}
		
	}
	
	void initialize() {
		m_draggable_window = std::make_shared<DraggableWindow>(*this, "");
		
		m_draggable_window->set_fixed_width(0);
		m_draggable_window->set_fixed_height(0);
		
		set_drag_widget(nullptr, nullptr);
	}
	
private:	
	void set_drag_widget(Widget* widget, std::function<void()> drag_callback) override {
		if(widget == nullptr){
			m_draggable_window->set_visible(false);
			m_draggable_window->set_drag_callback(nullptr);
			m_drag_active = false;
			m_drag_widget = nullptr;
			m_drag_callback = nullptr;
		} else {
			m_draggable_window->set_visible(false);
			m_draggable_window->set_drag_callback(drag_callback);
			m_drag_widget = m_draggable_window.get();
			m_drag_active = false;
			m_drag_callback = nullptr;
		}
	}
	Widget* drag_widget() const override { return m_draggable_window.get(); }

	void move_widget_to_top(Widget& widget) {
		auto &children = widget.parent()->get().children();
		auto it = std::find_if(children.begin(), children.end(), [&widget](std::reference_wrapper<Widget> item){
			return &item.get() == &widget;
		});
		if (it != children.end()) {
			// Move the widget to the end of the list to make it topmost
			children.erase(it);
			children.push_back(widget);
		}
	}
	
private:
	std::shared_ptr<DraggableWindow> m_draggable_window;

};

}  // namespace nanogui

class ICartridge;
class VirtualMachine;

class Actor;
class ActorManager;
class BlueprintManager;
class Canvas;
class CartridgeActorLoader;
class CameraManager;
class OpenAiApiClient;
class DeepMotionApiClient;
class ExecutionManager;
class GizmoManager;
class MeshActor;
class MeshActorLoader;
class MeshBatch;
class PowerAi;
class RenderCommon;
class ShaderManager;
class ShaderWrapper;
class SimulationServer;
class SkinnedMeshBatch;
class TripoAiApiClient;
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

   private:
	bool drop_event(Widget& sender, const std::vector<std::string> & filenames) override;

	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	
	
	AnimationTimeProvider mGlobalAnimationTimeProvider;
	AnimationTimeProvider mPreviewAnimationTimeProvider;
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
	std::unique_ptr<PowerAi> mPowerAi;
	std::unique_ptr<OpenAiApiClient> mOpenAiApiClient;
	std::unique_ptr<TripoAiApiClient> mTripoAiApiClient;
	std::unique_ptr<DeepMotionApiClient> mDeepMotionApiClient;
	std::unique_ptr<CameraManager> mCameraManager;
	std::unique_ptr<entt::registry> mEntityRegistry;
	std::unique_ptr<SimulationServer> mSimulationServer;

	std::queue<std::tuple<bool, int, int, int, int>> mClickQueue;
	std::vector<std::function<void(bool, int, int, int, int)>> mClickCallbacks;
	std::vector<std::function<void()>> mEventQueue;
};
