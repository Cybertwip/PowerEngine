#pragma once

#include "actors/IActorSelectedCallback.hpp"
#include "actors/IActorSelectedRegistry.hpp"
#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include "components/BlueprintComponent.hpp"
#include "components/BlueprintMetadataComponent.hpp"
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

class BlueprintPanel : public ScenePanel, public IActorSelectedCallback {
public:
	BlueprintPanel(Canvas& parent, std::shared_ptr<IActorSelectedRegistry> registry, ActorManager& actorManager, std::function<void(bool, std::function<void()>)> blueprintActionTriggerCallback)
	: ScenePanel(parent, "Blueprint")
	, mRegistry(registry)
	, mActorManager(actorManager)
	, mBlueprintActionTriggerCallback(blueprintActionTriggerCallback)
	, mActive(false)
	, mCommitted(true) {
		// Set the layout to horizontal with some padding
		set_layout(std::make_unique<nanogui::GroupLayout>(0, 0, 0));
		
		set_background_color(nanogui::Color(35, 65, 90, 224));
		
		mNodeProcessor = std::make_unique<NodeProcessor>();
		
		// Ensure the canvas is created with the correct dimensions or set it explicitly if needed
		mCanvas = std::make_unique<BlueprintCanvas>(*this, parent.screen(), *mNodeProcessor, [this](){
			on_canvas_modified();
		}, nanogui::Color(0, 0, 0, 0));
		
		mCanvas->set_fixed_size(nanogui::Vector2i(fixed_width(), parent.fixed_height() * 0.71));
		
		mBlueprintButton = std::make_shared<nanogui::Button>(parent, "", FA_FLASK);
		
		mBlueprintButton->set_fixed_size(nanogui::Vector2i(48, 48));
		
		mBlueprintButton->set_text_color(nanogui::Color(135, 206, 235, 255));
		
		mBlueprintButton->set_position(nanogui::Vector2i(parent.fixed_width() * 0.5f - mBlueprintButton->fixed_width() * 0.5f, parent.fixed_height() - mBlueprintButton->fixed_height() - 20));
		
		mBlueprintButton->set_callback([this](){
			if (!mCommitted) {
				commit();
			} else {
				mActive = !mActive;
				
				if (mActive) {
					if (mActiveActor) {
						deserialize(mActiveActor->get());
					}
				}
				
				mBlueprintActionTriggerCallback(mActive, [this](){
					clear();
				});
			}
		});
		
		mBlueprintButton->set_enabled(false);
		
		mRegistry->RegisterOnActorSelectedCallback(*this);
		
		
		mLoadBlueprintButton = std::make_unique<nanogui::Button>(std::ref(parent), "", FA_DOWNLOAD);
		mSaveBlueprintButton = std::make_unique<nanogui::Button>(std::ref(parent), "", FA_SAVE);
		
		const nanogui::Vector2i button_size(mButtonSize, mButtonSize);
		
		mLoadBlueprintButton->set_position(nanogui::Vector2i(0, parent.fixed_height()));
		mSaveBlueprintButton->set_position(nanogui::Vector2i(0, parent.fixed_height()));

		mLoadBlueprintButton->set_fixed_size(button_size);
		mSaveBlueprintButton->set_fixed_size(button_size);
		
		mLoadBlueprintButton->set_callback([this]() {
			nanogui::async([this]() {
				nanogui::file_dialog_async(
										   {{"bpn", "Blueprint Files"}}, false, false, [this](const std::vector<std::string>& files) {
											   if (files.empty()) {
												   return; // User canceled
											   }
											   
											   std::string destinationFile = files.front();
											   
											   if (mActiveActor.has_value()) {
												   if (mActiveActor->get().find_component<BlueprintComponent>()) {
													   
													   mActiveActor->get().get_component<BlueprintComponent>().load_blueprint(destinationFile);
													   
													   deserialize(mActiveActor->get());

												   }
												   
											   }
											   
										   });
			});
			
		});
		
		mSaveBlueprintButton->set_callback([this]() {
			nanogui::async([this]() {
				nanogui::file_dialog_async(
										   {{"bpn", "Blueprint Files"}}, true, false, [this](const std::vector<std::string>& files) {
											   if (files.empty()) {
												   return; // User canceled
											   }
											   
											   std::string destinationFile = files.front();
											   
											   if (mActiveActor.has_value()) {
												   if (mActiveActor->get().find_component<BlueprintComponent>()) {
													   
													   mActiveActor->get().get_component<BlueprintComponent>().save_blueprint(destinationFile);
													   
													   if (mActiveActor->get().find_component<BlueprintMetadataComponent>()) {
													
													
												   mActiveActor->get().remove_component<BlueprintMetadataComponent>();
												   }
												   auto& _ = mActiveActor->get().add_component<BlueprintMetadataComponent>(destinationFile);
												   }
												   
											   }

										   });
			});
			
			
		});
	}
	
	~BlueprintPanel() override {
		mRegistry->UnregisterOnActorSelectedCallback(*this);
	}
	
	void OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) override {
		mActiveActor = actor;
		
		if (mActiveActor.has_value()) {
			if (mActiveActor->get().find_component<BlueprintComponent>()) {
				deserialize(mActiveActor->get());
				mBlueprintButton->set_enabled(true);
			}
		} else {
			mBlueprintButton->set_enabled(false);
			mBlueprintButton->set_pushed(false);
			mBlueprintActionTriggerCallback(false, [this](){
				clear();
			});
		}
	}
	
	void on_canvas_modified() {		
		mBlueprintButton->set_text_color(nanogui::Color(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow

		mCommitted = false;
	}
	
	void commit() {
		if (mActiveActor.has_value()) {
			serialize(mActiveActor->get());
		}
		
		mCommitted = true;
		
		mBlueprintButton->set_text_color(nanogui::Color(135, 206, 235, 255));
	}

	
	void process_events() override {
		ScenePanel::process_events();
	}
	
	bool keyboard_event(int key, int scancode, int action, int modifiers) override {
		
		bool handled = nanogui::Widget::keyboard_event(key, scancode, action, modifiers);
		
		if (!handled) {
			if (key == GLFW_KEY_DELETE && action == GLFW_PRESS) {
				auto* selected_node = mCanvas->selected_node();
				
				if (selected_node) {
					mNodeProcessor->break_links(selected_node->core_node());
					mCanvas->clear_selection();
					return true;
				}
			}
		}
		
		return handled;
	}
	
	// Override mouse_button_event to consume the event
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {
		ScenePanel::mouse_button_event(p, button, down, modifiers);
		// delegate and consume event
		return true;
	}
	
	// Override mouse_motion_event to consume the event
	bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {
		ScenePanel::mouse_motion_event(p, rel, button, modifiers);
		// delegate and consume event
		return true;
	}
	
	void serialize(Actor& actor) {
		mNodeProcessor->serialize(*mCanvas, actor);
	}
	
	void deserialize(Actor& actor) {
		mNodeProcessor->deserialize(*mCanvas, actor);
		mCommitted = true;
		mBlueprintButton->set_text_color(nanogui::Color(135, 206, 235, 255));
	}
	
	void clear() {
		mCanvas->clear();
		mNodeProcessor->clear();
	}
	
	void start() {
		mBlueprintActors = mActorManager.get_actors_with_component<BlueprintComponent>();
	}
	
	void stop() {
		mBlueprintActors.clear();
	}
	
	void update() {
		for (auto& actor : mBlueprintActors) {
			actor.get().get_component<BlueprintComponent>().update(); //might be slow with thousands of objets due to get_component
		}
	}
	
	void update_buttons(const nanogui::Vector2f& position) {
		mLoadBlueprintButton->set_position(nanogui::Vector2i(position.x(), position.y() + padding));
		mSaveBlueprintButton->set_position(nanogui::Vector2i(mButtonSize + position.x() + padding, position.y() + padding));
	}

private:
	void draw(NVGcontext *ctx) override {
		ScenePanel::draw(ctx);
	}
	
private:
	std::unique_ptr<BlueprintCanvas> mCanvas;
	std::unique_ptr<NodeProcessor> mNodeProcessor;
	std::shared_ptr<IActorSelectedRegistry> mRegistry;
	ActorManager& mActorManager;

	std::function<void(bool, std::function<void()>)> mBlueprintActionTriggerCallback;
	
	std::shared_ptr<nanogui::Button> mBlueprintButton;

	std::optional<std::reference_wrapper<Actor>> mActiveActor;
	
	std::vector<std::reference_wrapper<Actor>> mBlueprintActors;

	bool mActive;
	bool mCommitted;
	
	const int mButtonSize = 32;
	const int padding = 15;

	std::unique_ptr<nanogui::Button> mLoadBlueprintButton;
	std::unique_ptr<nanogui::Button> mSaveBlueprintButton;

};

class BlueprintManager {
public:
	BlueprintManager(Canvas& canvas, std::shared_ptr<IActorSelectedRegistry> registry, ActorManager& actorManager)
	: mCanvas(canvas) {
		mBlueprintPanel = std::make_shared<BlueprintPanel>(canvas, registry, actorManager, [this](bool active, std::function<void()> onPanelToggle){
			toggle_blueprint_panel(active, onPanelToggle);
		});
		
		mBlueprintPanel->set_position(nanogui::Vector2i(0, canvas.fixed_height()));
		
	}
	
	bool keyboard_event(int key, int scancode, int action, int modifiers) {
		return mBlueprintPanel->keyboard_event(key, scancode, action, modifiers);
	}

	void toggle_blueprint_panel(bool active, std::function<void()> onPanelToggle) {
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
			mAnimationFuture = std::async(std::launch::async, [this, onPanelToggle]() {
				auto target = nanogui::Vector2i(0, mCanvas.fixed_height());
				animate_panel_position(target);
				
				mBlueprintPanel->set_visible(false);
				onPanelToggle();
			});
		}
	}
	
	void animate_panel_position(const nanogui::Vector2i &targetPosition) {
		const int steps = 20;
		const auto stepDelay = std::chrono::milliseconds(16); // ~60 FPS
		nanogui::Vector2f startPos = mBlueprintPanel->position();
		nanogui::Vector2f buttonPadding = nanogui::Vector2f(startPos.x(), startPos.y() + padding);
		nanogui::Vector2f step = (nanogui::Vector2f(targetPosition) - startPos) / (steps - 1);
		
		for (int i = 0; i < steps; ++i) {
			// This check ensures we can stop the animation if another toggle happens
			if (mAnimationFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
				mBlueprintPanel->set_position(startPos + step * i);
				mBlueprintPanel->update_buttons(buttonPadding + step * i);
				std::this_thread::sleep_for(stepDelay);
			} else {
				break; // Stop animation if future is no longer valid (another animation started)
			}
		}
	}
	
	void start() {
		mBlueprintPanel->start();
	}
	
	void stop() {
		mBlueprintPanel->stop();
	}
	
	void update() {
		mBlueprintPanel->update();
	}
	
	void commit() {
		mBlueprintPanel->commit();
	}
	
	
	void process_events() {
		mBlueprintPanel->process_events();
	}
	
private:
	Canvas& mCanvas;

	std::shared_ptr<BlueprintPanel> mBlueprintPanel;
	std::future<void> mAnimationFuture;
	
	const int padding = 15;
};
