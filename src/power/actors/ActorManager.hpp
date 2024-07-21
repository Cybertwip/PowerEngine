#pragma once

#include <memory>
#include <string>
#include <vector>

class Actor;
class CameraManager;
class MeshActor;
class MeshActorLoader;

class ActorManager {
public:
    ActorManager(CameraManager& cameraManager);
    void push(Actor& actor);
    void draw();
    
private:
    CameraManager& mCameraManager;
    std::vector<std::reference_wrapper<Actor>> mActors;
};

