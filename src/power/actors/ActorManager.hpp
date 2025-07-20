#pragma once

#include "IActorManager.hpp"

#include "actors/Actor.hpp"

#include <entt/entt.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class CameraManager;
class MeshActor;
class MeshActorLoader;
class GizmoManager;
class UiManager;
class Batch;
class SerializationModule;

class ActorManager : public IActorManager {
public:
    ActorManager(entt::registry& registry, CameraManager& cameraManager);
	Actor& create_actor() override;
	void remove_actor(Actor& actor) override;
	void remove_actors(const std::vector<std::reference_wrapper<Actor>>& actors) override;
	
	void clear_actors();

	template<typename T>
	const std::vector<std::reference_wrapper<Actor>> get_actors_with_component() const {
		
		std::vector<std::reference_wrapper<Actor>> actors;
		for (auto& actor : mActors) {
			if (actor->find_component<T>()) {
				actors.push_back(*actor);
			}
		}
		
		return actors;
	}

    void draw();
	void visit(GizmoManager& gizmoManager);
	void visit(UiManager& uiManager);
	void visit(Batch& batch);
private:
	Actor& create_actor(entt::entity entity);
	entt::registry& registry() {
		return mRegistry;
	}
	
	const std::vector<std::reference_wrapper<Actor>> get_actors() const {
		
		for (auto& actor : mActors) {
			actors.push_back(*actor);
		}
		
		return actors;
	}


	entt::registry& mRegistry;
    CameraManager& mCameraManager;
    std::vector<std::unique_ptr<Actor>> mActors;

private:
	friend class SerializationModule;
	friend class HierarchyPanel;

};

