#include "CameraManager.hpp"

#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

CameraManager::CameraManager(entt::registry& registry)
    : mRegistry(registry), mActiveCamera(std::nullopt) {}

void CameraManager::update_from(const ActorManager& actorManager) {
	auto cameras = actorManager.get_actors_with_component<CameraComponent>();
	
	mCameras.clear();
	
	for (auto& camera : cameras) {
		mCameras.push_back(camera);
	}
	
	if (mActiveCamera.has_value()) {
		auto it = std::find_if(mCameras.begin(), mCameras.end(), [this](auto& camera){
			if (&camera.get() == &mActiveCamera->get()) {
				return true;
			} else {
				return false;
			}
		});
		
		if (it == mCameras.end()) {
			mActiveCamera = std::nullopt;
		}
	} else {
		mActiveCamera = *mCameras.begin();
	}

}

void CameraManager::update_view() {
    if (mActiveCamera.has_value()) {
		mActiveCamera->get().get_component<CameraComponent>().update_view();
    }
}

const nanogui::Matrix4f CameraManager::get_view() const {
    if (mActiveCamera.has_value()) {
        return mActiveCamera->get().get_component<CameraComponent>().get_view();
    } else {
        return nanogui::Matrix4f();
    }
}
const nanogui::Matrix4f CameraManager::get_projection() const {
    if (mActiveCamera.has_value()) {
		return mActiveCamera->get().get_component<CameraComponent>().get_projection();
    } else {
        return nanogui::Matrix4f::perspective(45, 0.01f, 5e3f, 16.0f / 9.0f);
    }
}

void CameraManager::look_at(Actor& actor) {
    if (mActiveCamera.has_value()) {
		mActiveCamera->get().get_component<CameraComponent>().look_at(actor);
    }
}
