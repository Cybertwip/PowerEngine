#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Transform {
public:
	glm::vec3 translation;
	glm::quat rotation;
	glm::vec3 scale;
	
	Transform()
	: translation(0.0f), rotation(1.0f, 0.0f, 0.0f, 0.0f), scale(1.0f) {}
	
	// Copy constructor
	Transform(const Transform& other) = default;
	
	// Copy assignment operator
	Transform& operator=(const Transform& other) = default;
	
	static Transform from_matrix(const glm::mat4& matrix) {
		Transform transform;
		
		// Extract translation
		transform.translation = glm::vec3(matrix[3]);
		
		// Extract scale factors
		glm::vec3 scale;
		scale.x = glm::length(glm::vec3(matrix[0]));
		scale.y = glm::length(glm::vec3(matrix[1]));
		scale.z = glm::length(glm::vec3(matrix[2]));
		transform.scale = scale;
		
		// Remove scale from the matrix to extract rotation
		glm::mat4 rotationMatrix = matrix;
		
		if (scale.x != 0) rotationMatrix[0] /= scale.x;
		if (scale.y != 0) rotationMatrix[1] /= scale.y;
		if (scale.z != 0) rotationMatrix[2] /= scale.z;
		
		transform.rotation = glm::quat_cast(rotationMatrix);
		
		return transform;
	}

	glm::mat4 to_matrix() const {
		glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), translation);
		glm::mat4 rotation_matrix = glm::mat4_cast(rotation);  // Convert quaternion to matrix
		glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale);
		return translation_matrix * rotation_matrix * scale_matrix;
	}

};
