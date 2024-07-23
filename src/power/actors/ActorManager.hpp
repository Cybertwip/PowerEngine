#pragma once

#include <tiny-gizmo.hpp>

#include <memory>
#include <string>
#include <vector>

class Actor;
class CameraManager;
class MeshActor;
class MeshActorLoader;
class GizmoManager;

class ActorManager {
public:
    ActorManager(CameraManager& cameraManager);
    void push(Actor& actor);
    void draw();
    void visit(GizmoManager& gizmoManager);

private:
    CameraManager& mCameraManager;
    std::vector<std::reference_wrapper<Actor>> mActors;

};

