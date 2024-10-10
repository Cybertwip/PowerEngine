#pragma once

#include "ICameraManager.hpp"
#include "actors/IActorSelectedCallback.hpp"

#include <entt/entt.hpp>

#include <nanogui/vector.h>

#include <memory>

class Actor;
class ActorManager;

class IActorSelectedRegistry;
class ScenePanel;

class CameraManager : public IActorSelectedCallback, public ICameraManager
{
public:
    CameraManager(entt::registry& registry);
	
	~CameraManager();
	
	void update_from(const ActorManager& actorManager);
    
    std::optional<std::reference_wrapper<Actor>> active_camera() { return mActiveCamera; }
    
    void update_view();
    
    const nanogui::Matrix4f get_view() const;
    const nanogui::Matrix4f get_projection() const;
    
    void look_at(Actor& actor) override;
	
	void look_at(const glm::vec3& position) override;

	void set_transform(const glm::mat4& transform) override;

	std::optional<glm::mat4> get_transform() override;
	
	void rotate_camera(float dx, float dy);
	void zoom_camera(float dy);
	void pan_camera(float dx, float dy);


private:
	
	void OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) override;

    entt::registry& mRegistry;
    std::optional<std::reference_wrapper<Actor>> mActiveCamera;
	std::vector<std::reference_wrapper<Actor>> mCameras;
	
	std::optional<std::reference_wrapper<Actor>> mActiveActor;
};
