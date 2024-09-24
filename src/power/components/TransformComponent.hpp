#pragma once

#include "animation/Transform.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <nanogui/vector.h>
#include <functional>
#include <unordered_map>
#include <iostream>

class TransformComponent {
public:
	using TransformChangedCallback = std::function<void(const TransformComponent&)>;
	
	static nanogui::Matrix4f glm_to_nanogui(glm::mat4 glmMatrix) {
		nanogui::Matrix4f matrix;
		std::memcpy(matrix.m, glm::value_ptr(glmMatrix), sizeof(float) * 16);
		return matrix;
	}
	
	static glm::mat4 nanogui_to_glm(const nanogui::Matrix4f& nanoguiMatrix) {
		glm::mat4 glmMatrix;
		std::memcpy(glm::value_ptr(glmMatrix), nanoguiMatrix.m, sizeof(float) * 16);
		return glmMatrix;
	}
	
	Transform transform;
	
	TransformComponent() {
		transform.translation = glm::vec3(0.0f, 0.0f, 0.0f);
		transform.rotation = glm::quat(1, 0, 0, 0);
		transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
	}
	
	// Callback registration, returns an ID for future unregistration
	int register_on_transform_changed_callback(TransformChangedCallback callback) {
		int id = nextCallbackId++;
		callbacks[id] = callback;
		return id;
	}
	
	// Unregister a callback using its ID
	void unregister_on_transform_changed_callback(int id) {
		callbacks.erase(id);
	}
	
	void set_translation(const glm::vec3& translation) {
		transform.translation = translation;
		trigger_on_transform_changed();
	}
	
	void set_rotation(const glm::quat& rotation) {
		transform.rotation = rotation;
		trigger_on_transform_changed();
	}
	
	void set_scale(const glm::vec3& scale) {
		transform.scale = scale;
		trigger_on_transform_changed();
	}
	
	glm::vec3 get_translation() const {
		return glm::vec3(transform.translation.x, transform.translation.y, transform.translation.z);
	}
	
	glm::vec3 get_scale() const {
		return glm::vec3(transform.scale.x, transform.scale.y, transform.scale.z);
	}
	
	glm::quat get_rotation() const {
		return glm::quat(transform.rotation.w, transform.rotation.x, transform.rotation.y, transform.rotation.z);
	}
	
	glm::mat4 get_matrix() const {
		glm::vec3 position = get_translation();
		glm::quat rotation = get_rotation();
		glm::vec3 scale = get_scale();
		
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), { position.x, position.y, position.z });
		glm::mat4 rotationMatrix = glm::mat4_cast(glm::conjugate(rotation));
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
		
		return translationMatrix * rotationMatrix * scaleMatrix;
	}
	
	void rotate(const glm::vec3& axis, float angle) {
		glm::quat rotationQuat = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
		glm::quat currentRotation = get_rotation();
		glm::quat newRotation = currentRotation * rotationQuat;
		set_rotation(newRotation);
	}
	
private:
	// Use an unordered_map to store callbacks with an integer key (ID)
	std::unordered_map<int, TransformChangedCallback> callbacks;
	int nextCallbackId = 0;
	
	// Trigger all callbacks when the transform changes
	void trigger_on_transform_changed() {
		for (const auto& [id, callback] : callbacks) {
			callback(*this);
		}
	}
};
