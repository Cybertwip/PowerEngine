#include "CameraManager.hpp"

#include "actors/Actor.hpp"
#include "actors/ActorManager.hpp"
#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

CameraManager::CameraManager(entt::registry& registry)
    : mRegistry(registry), mActiveCamera(std::nullopt) {

}

CameraManager::~CameraManager() {
}

void CameraManager::update_from(const ActorManager& actorManager) {
	auto cameras = actorManager.get_actors_with_component<CameraComponent>();
	
	mCameras.clear();
	
	for (auto& camera : cameras) {
		mCameras.push_back(camera);
	}
	
	mActiveCamera = *mEngineCamera;
}

void CameraManager::update_view() {
    if (mActiveCamera.has_value()) {
		mActiveCamera->get().get_component<CameraComponent>().update_view();
		
		mLastView = mActiveCamera->get().get_component<CameraComponent>().get_view();
		
		mLastProjection = mActiveCamera->get().get_component<CameraComponent>().get_projection();
		
    }
}

const nanogui::Matrix4f CameraManager::get_view() const {
    if (mActiveCamera.has_value()) {
        return mActiveCamera->get().get_component<CameraComponent>().get_view();
    } else {
		return mLastView;
    }
}
const nanogui::Matrix4f CameraManager::get_projection() const {
    if (mActiveCamera.has_value()) {
		return mActiveCamera->get().get_component<CameraComponent>().get_projection();
    } else {
		return mLastProjection;
	}
}

void CameraManager::look_at(Actor& actor) {
    if (mActiveCamera.has_value()) {
		mActiveCamera->get().get_component<CameraComponent>().look_at(actor);
    }
}

void CameraManager::look_at(const glm::vec3& position) {
	if (mActiveCamera.has_value()) {
		mActiveCamera->get().get_component<CameraComponent>().look_at(position);
	}
}


void CameraManager::OnActorSelected(std::optional<std::reference_wrapper<Actor>> actor) {
	
	if (actor) {
		if (actor->get().find_component<CameraComponent>()) {
			
			if (actor->get().get_component<CameraComponent>().active()) {
				mActiveCamera = actor;
				
				mLastView = mActiveCamera->get().get_component<CameraComponent>().get_view();
				
				mLastProjection = mActiveCamera->get().get_component<CameraComponent>().get_projection();
				
				for (auto& camera : mCameras) {
					camera.get().get_component<CameraComponent>().set_active(false);
				}
				
				actor->get().get_component<CameraComponent>().set_active(true);
			} else {
				mActiveCamera = *mEngineCamera;
			}
		} else {
			mActiveCamera = *mEngineCamera;
		}
	}
}

std::optional<std::reference_wrapper<TransformComponent>> CameraManager::get_transform_component() {
	if (mActiveCamera.has_value()) {
		return mActiveCamera->get().get_component<TransformComponent>();
	} else {
		return std::nullopt;
	}
}

void CameraManager::set_transform(const glm::mat4& transform) {
	if (mActiveCamera.has_value()) {
			auto& cameraTransform = mActiveCamera->get().get_component<TransformComponent>();
			
			glm::vec3 scale;
			glm::quat rotation;
			glm::vec3 translation;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose(transform, scale, rotation, translation, skew, perspective);

			cameraTransform.set_translation(translation);
			cameraTransform.set_rotation(rotation);
			cameraTransform.set_scale(scale);
	}
}

// Rotate the camera based on mouse movement
void CameraManager::rotate_camera(float dx, float dy) {
	if (mActiveCamera.has_value()) {
		auto& cameraActor = mActiveCamera->get();
		auto& transform = cameraActor.get_component<TransformComponent>();
		
		// Define rotation sensitivity
		float sensitivity = 0.05f; // Adjust as needed
		
		// Calculate rotation angles
		float yaw = -dx * sensitivity;
		float pitch = -dy * sensitivity;
		
		// Get the current rotation
		glm::quat rotation = transform.get_rotation();
		
		// Create quaternions for yaw and pitch
		glm::quat qYaw = glm::angleAxis(glm::radians(yaw), glm::vec3(0, 1, 0));   // Yaw around y-axis
		glm::quat qPitch = glm::angleAxis(glm::radians(pitch), glm::vec3(1, 0, 0)); // Pitch around X-axis
		
		// Update the rotation
		rotation = qYaw * rotation * qPitch;
		
		// Normalize the quaternion
		rotation = glm::normalize(rotation);
		
		// Set the new rotation
		transform.set_rotation(rotation);
		
		// Update the view matrix
		update_view();
	}
}

// Zoom the camera based on mouse movement
void CameraManager::zoom_camera(float dy) {
	if (mActiveCamera.has_value()) {
		auto& cameraActor = mActiveCamera->get();
		auto& transform = cameraActor.get_component<TransformComponent>();
		
		// Define zoom sensitivity
		float sensitivity = 1; // Adjust as needed
		
		// Get the forward direction
		glm::vec3 forward = transform.get_forward();
		
		// Calculate zoom amount
		glm::vec3 zoom = forward * dy * sensitivity;
		
		// Update the position
		glm::vec3 position = transform.get_translation();
		position += zoom;
		
		// Set the new position
		transform.set_translation(position);
		
		// Update the view matrix
		update_view();
	}
}

// Pan the camera based on mouse movement
void CameraManager::pan_camera(float dx, float dy) {
	if (mActiveCamera.has_value()) {
		auto& cameraActor = mActiveCamera->get();
		auto& transform = cameraActor.get_component<TransformComponent>();
		
		// Define pan sensitivity
		float sensitivity = 0.1f; // Adjust as needed
		
		// Get the right and up vectors
		glm::vec3 right = transform.get_right();
		glm::vec3 up = transform.get_up();
		
		// Calculate pan amounts
		glm::vec3 panHorizontal = -dx * sensitivity * right;
		glm::vec3 panVertical = dy * sensitivity * up;
		
		// Update the position
		glm::vec3 position = transform.get_translation();
		position += panHorizontal + panVertical;
		
		// Set the new position
		transform.set_translation(position);
		
		// Update the view matrix
		update_view();
	}
}

Actor& CameraManager::create_engine_camera(AnimationTimeProvider& animationTimeProvider, float fov, float near, float far, float aspect){
	
	mEngineCamera = std::make_unique<Actor>(mRegistry);
	
	CameraActorBuilder::build(*mEngineCamera, animationTimeProvider, fov, near, far, aspect);
	
	mActiveCamera = *mEngineCamera;
	
	mEngineCamera->get_component<CameraComponent>().set_active(true);

	return *mEngineCamera;
}
