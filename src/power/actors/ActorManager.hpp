#pragma once

#include "IActorManager.hpp"

#include "actors/Actor.hpp"

#include <entt/entt.hpp>

#include <memory>
#include <string>
#include <vector>

class CameraManager;
class MeshActor;
class MeshActorLoader;
class GizmoManager;
class UiManager;
class Batch;

class ActorManager : IActorManager {
public:
    ActorManager(entt::registry& registry, CameraManager& cameraManager);
	Actor& create_actor() override;
	void remove_actor(Actor& actor) override;

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
	entt::registry& mRegistry;
    CameraManager& mCameraManager;
    std::vector<std::unique_ptr<Actor>> mActors;

};

