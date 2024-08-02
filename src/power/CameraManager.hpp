#pragma once

#include <entt/entt.hpp>

#include <nanogui/vector.h>

#include <memory>

class Actor;
class ActorManager;

class CameraManager
{
public:
    CameraManager(entt::registry& registry);
	
	void update_from(const ActorManager& actorManager);
    
    std::optional<std::reference_wrapper<Actor>> active_camera() { return mActiveCamera; }
    
    void update_view();
    
    const nanogui::Matrix4f get_view() const;
    const nanogui::Matrix4f get_projection() const;
    
    void look_at(Actor& actor);

private:
    entt::registry& mRegistry;
    std::optional<std::reference_wrapper<Actor>> mActiveCamera;
	
	std::vector<std::reference_wrapper<Actor>> mCameras;
};
