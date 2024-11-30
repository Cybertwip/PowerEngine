#pragma once

#include "actors/IActorSelectedCallback.hpp"
#include "actors/IActorSelectedRegistry.hpp"
#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include "components/BlueprintComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/drawing/Grid.hpp"
#include "simulation/SimulationServer.hpp"
#include "ui/ScenePanel.hpp"

#include "BlueprintCanvas.hpp"
#include "NodeProcessor.hpp"
#include "BlueprintNode.hpp"
#include "ShaderManager.hpp"
#include "StringNode.hpp"
#include "PrintNode.hpp"

class BlueprintPanel : public ScenePanel {
public:
	BlueprintPanel(Canvas& parent)
	: ScenePanel(parent, "Blueprint") {
		// Set the layout to horizontal with some padding
		set_layout(std::make_unique<nanogui::GroupLayout>(0, 0, 0));
		
		set_background_color(nanogui::Color(35, 65, 90, 224));
		
		mNodeProcessor = std::make_unique<NodeProcessor>();
		
		// Ensure the canvas is created with the correct dimensions or set it explicitly if needed
		mCanvas = std::make_unique<BlueprintCanvas>(*this, parent.screen(), *mNodeProcessor, nanogui::Color(0, 0, 0, 0));
		
		mCanvas->set_fixed_size(nanogui::Vector2i(fixed_width(), parent.fixed_height() * 0.71));
	}
	
	void process_events() override {
		ScenePanel::process_events();
	}
	
	bool keyboard_event(int key, int scancode, int action, int modifiers) override {
		
		if (key == GLFW_KEY_DELETE && action == GLFW_PRESS) {
			auto* selected_node = mCanvas->selected_node();
			
			if (selected_node) {
				mNodeProcessor->break_links(selected_node);
				mCanvas->clear_selection();
				return true;
			} else {
				return false;
			}

		} else {
			return false;
		}
	}
	
	// Override mouse_button_event to consume the event
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {
		nanogui::Widget::mouse_button_event(p, button, down, modifiers);
		
		return true;
	}
	
	// Override mouse_motion_event to consume the event
	bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {
		nanogui::Widget::mouse_motion_event(p, rel, button, modifiers);
		
		return true;
	}
	
	void serialize(Actor& actor) {
		mNodeProcessor->serialize(*mCanvas, actor);
		clear();
	}
	
	void deserialize(Actor& actor) {
		clear();
		mNodeProcessor->deserialize(*mCanvas, actor);
	}
	
	void clear() {
		mCanvas->clear();
		mNodeProcessor->clear();
	}

private:
	void draw(NVGcontext *ctx) override {
		ScenePanel::draw(ctx);
	}
	
private:
	std::unique_ptr<BlueprintCanvas> mCanvas;
	std::unique_ptr<NodeProcessor> mNodeProcessor;
};

class BlueprintManager : public IActorSelectedCallback {
public:
	BlueprintManager(Canvas& canvas, std::shared_ptr<IActorSelectedRegistry> registry, ActorManager& actorManager)
	: mCanvas(canvas)
	, mRegistry(registry)
	, mActorManager(actorManager) {
		mBlueprintPanel = std::make_shared<BlueprintPanel>(canvas);
		
		mBlueprintPanel->set_position(nanogui::Vector2i(0, canvas.fixed_height()));
		
		mBlueprintButton = std::make_shared<nanogui::ToolButton>(canvas, FA_FLASK);
		
		mBlueprintButton->set_fixed_size(nanogui::Vector2i(48, 48));
		
		mBlueprintButton->set_text_color(nanogui::Color(135, 206, 235, 255));
		
		// Position the button in the lower-right corner
		mBlueprintButton->set_position(nanogui::Vector2i(mCanvas.fixed_width() * 0.5f - mBlueprintButton->fixed_width() * 0.5f, mCanvas.fixed_height() - mBlueprintButton->fixed_height() - 20));
		
		mBlueprintButton->set_change_callback([this](bool active) {
			toggle_blueprint_panel(active);
		});
		
		mBlueprintButton->set_enabled(false);
		
		mRegistry->RegisterOnActorSelectedCallback(*this);

	}
	
	~BlueprintManager() {
		mRegistry->UnregisterOnActorSelectedCallback(*this);
	}
	
	void OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) override {

		mActiveActor = actor;
		
		if (mActiveActor.has_value()) {
			mBlueprintPanel->deserialize(mActiveActor->get());
			mBlueprintButton->set_enabled(true);
		} else {
			mBlueprintButton->set_enabled(false);
			mBlueprintButton->set_pushed(false);
			toggle_blueprint_panel(false);
			mBlueprintPanel->clear();
		}
	}
	
	void commit_blueprint() {
		if (mActiveActor.has_value()) {
			mBlueprintPanel->serialize(mActiveActor->get());
		}
	}
	
	bool keyboard_event(int key, int scancode, int action, int modifiers) {
		return mBlueprintPanel->keyboard_event(key, scancode, action, modifiers);
	}

	void toggle_blueprint_panel(bool active) {
		if (mAnimationFuture.valid() && mAnimationFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
			return; // Animation is still running, do not start a new one
		}
		
		mBlueprintPanel->set_visible(true);

		if (active) {
			mAnimationFuture = std::async(std::launch::async, [this]() {
				auto target = nanogui::Vector2i(0, mCanvas.fixed_height() * 0.25f);
				animate_panel_position(target);
			});
		} else {
			mAnimationFuture = std::async(std::launch::async, [this]() {
				auto target = nanogui::Vector2i(0, mCanvas.fixed_height());
				animate_panel_position(target);
				
				mBlueprintPanel->set_visible(false);
			});
		}
	}
	
	void animate_panel_position(const nanogui::Vector2i &targetPosition) {
		const int steps = 20;
		const auto stepDelay = std::chrono::milliseconds(16); // ~60 FPS
		nanogui::Vector2f startPos = mBlueprintPanel->position();
		nanogui::Vector2f step = (nanogui::Vector2f(targetPosition) - startPos) / (steps - 1);
		
		for (int i = 0; i < steps; ++i) {
			// This check ensures we can stop the animation if another toggle happens
			if (mAnimationFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
				mBlueprintPanel->set_position(startPos + step * i);
				std::this_thread::sleep_for(stepDelay);
			} else {
				break; // Stop animation if future is no longer valid (another animation started)
			}
		}
	}
	
	void start() {
		mBlueprintActors = mActorManager.get_actors_with_component<BlueprintComponent>();
	}
	
	void stop() {
		mBlueprintActors.clear();
	}
	
	void process_events() {
		mBlueprintPanel->process_events();
	}
	
	void update() {
		for (auto& actor : mBlueprintActors) {
			actor.get().get_component<BlueprintComponent>().update(); //might be slow with thousands of objets due to get_component
		}
	}
	
private:
	Canvas& mCanvas;
	std::shared_ptr<IActorSelectedRegistry> mRegistry;
	ActorManager& mActorManager;

	std::shared_ptr<BlueprintPanel> mBlueprintPanel;
	std::shared_ptr<nanogui::ToolButton> mBlueprintButton;
	std::future<void> mAnimationFuture;
	
	std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	std::vector<std::reference_wrapper<Actor>> mBlueprintActors;
	
};
