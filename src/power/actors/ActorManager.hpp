#pragma once

#include <entt/entt.hpp>

#include <memory>
#include <string>
#include <vector>

class Actor;
class CameraManager;
class MeshActor;
class MeshActorLoader;
class GizmoManager;
class UiManager;

class ActorManager {
public:
    ActorManager(entt::registry& registry, CameraManager& cameraManager);
	Actor& create_actor();

    void draw();
	void visit(GizmoManager& gizmoManager);
	void visit(UiManager& uiManager);

private:
	entt::registry& mRegistry;
    CameraManager& mCameraManager;
    std::vector<std::unique_ptr<Actor>> mActors;

};

