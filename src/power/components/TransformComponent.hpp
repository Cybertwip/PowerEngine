#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ozz/base/maths/transform.h>

#include <nanogui/vector.h>

class TransformComponent {
public:
    static nanogui::Matrix4f glm_to_nanogui(glm::mat4 glmMatrix){
        
        nanogui::Matrix4f matrix;
        
        // Convert glm::mat4 to nanogui::Matrix4f
        std::memcpy(matrix.m, glm::value_ptr(glmMatrix), sizeof(float) * 16);
        
        return matrix;

    }
	
	static glm::mat4 nanogui_to_glm(const nanogui::Matrix4f& nanoguiMatrix) {
		glm::mat4 glmMatrix;
		
		// Assuming both glm::mat4 and nanogui::Matrix4f are stored in column-major order
		std::memcpy(glm::value_ptr(glmMatrix), nanoguiMatrix.m, sizeof(float) * 16);
		
		return glmMatrix;
	}

    ozz::math::Transform transform;

    TransformComponent() {
        transform.translation = ozz::math::Float3(0.0f, 0.0f, 0.0f);
        transform.rotation = ozz::math::Quaternion(0, 0, 0, 1);
        transform.scale = ozz::math::Float3(1.0f, 1.0f, 1.0f);
    }

    void set_translation(const glm::vec3& translation) {
        transform.translation = ozz::math::Float3(translation.x, translation.y, translation.z);
    }

    void set_rotation(const glm::quat& rotation) {
        transform.rotation = ozz::math::Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
    }
	
	void set_scale(const glm::vec3& scale) {
		transform.scale = ozz::math::Float3(scale.x, scale.y, scale.z);
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
        // Convert ozz::math::Transform to glm types
        glm::vec3 position = get_translation();
        glm::quat rotation = get_rotation();
        glm::vec3 scale = get_scale();

        // Calculate the model matrix
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), {position.x, position.y, -position.z});
        glm::mat4 rotationMatrix = glm::mat4_cast(glm::conjugate(rotation));
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

        return translationMatrix * rotationMatrix * scaleMatrix;
    }
	
	void rotate(const glm::vec3& axis, float angle) {
		// Create a quaternion representing the rotation
		glm::quat rotationQuat = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
		
		// Get the current rotation
		glm::quat currentRotation = get_rotation();
		
		// Combine the current rotation with the new rotation
		glm::quat newRotation = rotationQuat * currentRotation;
		
		// Set the new rotation
		set_rotation(newRotation);
	}
};