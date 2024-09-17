// HierarchyPanel.cpp
#include "ui/HierarchyPanel.hpp"

#include "ui/ScenePanel.hpp"
#include "ui/TransformPanel.hpp"

#include <nanogui/layout.h>
#include <nanogui/opengl.h>
#include <nanogui/treeviewitem.h>

#include "actors/Actor.hpp"
#include "actors/IActorSelectedCallback.hpp"
#include "components/MetadataComponent.hpp"
#include "components/UiComponent.hpp"

HierarchyPanel::HierarchyPanel(ScenePanel& scenePanel, TransformPanel& transformPanel, nanogui::Widget &parent) : Panel(parent, "Hierarchy"), mTransformPanel(transformPanel) {
	set_position(nanogui::Vector2i(0, 0));
	set_layout(new nanogui::GroupLayout());
	
	mScrollPanel = new nanogui::VScrollPanel(this);
	
	mScrollPanel->set_fixed_size({0, 12 * 25});
	
	mTreeView = new nanogui::TreeView(mScrollPanel);
	mTreeView->set_layout(
						  new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Fill));
}

bool HierarchyPanel::mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
									  int button, int modifiers) {
	// Disable dragging
	return mScrollPanel->mouse_drag_event(p, rel, button, modifiers);
}

void HierarchyPanel::add_actors(const std::vector<std::reference_wrapper<Actor>> &actors) {
	for (auto &actor : actors) {
		populate_tree(actor.get());
	}
}

void HierarchyPanel::add_actor(std::reference_wrapper<Actor> actor) {
	populate_tree(actor.get());
}

void HierarchyPanel::populate_tree(Actor &actor, nanogui::TreeViewItem *parent_node) {
	// Correctly reference the actor's name
	nanogui::TreeViewItem *node =
	parent_node
	? parent_node->add_node(
							std::string{actor.get_component<MetadataComponent>().name()},
							[this, &actor]() {
								OnActorSelected(actor);
							})
	: mTreeView->add_node(std::string{actor.get_component<MetadataComponent>().name()},
						  [this, &actor]() {
		OnActorSelected(actor);
	});
	// Uncomment and correctly iterate over the actor's children
	//    for (Actor *child : actor->children()) {
	//        populate_tree(child, node);
	//    }
	
	
	if (actor.find_component<UiComponent>()) {
		actor.remove_component<UiComponent>();
	}
	
	actor.add_component<UiComponent>([this, &actor, node]() {
		mTreeView->set_selected(node);
		OnActorSelected(actor);
	});
	
	perform_layout(screen()->nvg_context());
}

void HierarchyPanel::RegisterOnActorSelectedCallback(IActorSelectedCallback& callback) {
	if (mActorSelectedCallbacks.size() >= 2) {
		throw std::runtime_error("Cannot register more than two callbacks.");
	}
	mActorSelectedCallbacks.push_back(std::ref(callback));
}

void HierarchyPanel::UnregisterOnActorSelectedCallback(IActorSelectedCallback& callback) {
	auto it = std::remove_if(mActorSelectedCallbacks.begin(), mActorSelectedCallbacks.end(),
							 [&callback](std::reference_wrapper<IActorSelectedCallback>& ref) {
		return &ref.get() == &callback;
	});
	if (it != mActorSelectedCallbacks.end()) {
		mActorSelectedCallbacks.erase(it, mActorSelectedCallbacks.end());
	}
}

void HierarchyPanel::OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) {
	
	if (actor.has_value()) {
		mTransformPanel.set_active_actor(actor);
		
		for (auto& callbackRef : mActorSelectedCallbacks) {
			callbackRef.get().OnActorSelected(actor);
		}

	} else {
		mTreeView->set_selected(nullptr);
	}
	
}

void HierarchyPanel::fire_actor_selected_event(std::optional<std::reference_wrapper<Actor>> actor) {
	
	if (actor.has_value()) {
		mTransformPanel.set_active_actor(actor);
		
		for (auto& callbackRef : mActorSelectedCallbacks) {
			callbackRef.get().OnActorSelected(actor);
		}
		
	} else {
		mTreeView->set_selected(nullptr);
	}
}




