#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform {
public:
	// Translation affine transformation component
	glm::vec3 translation;
	
	// Rotation affine transformation component (represented as a quaternion)
	glm::quat rotation;
	
	// Scale affine transformation component
	glm::vec3 scale;
	
	// Default constructor - initializes to identity transform
	Transform()
	: translation(0.0f, 0.0f, 0.0f),  // No translation
	rotation(glm::identity<glm::quat>()),  // No rotation (identity quaternion)
	scale(1.0f, 1.0f, 1.0f)  // Uniform scale of 1.0
	{}
	
	// Constructor with specified translation, rotation, and scale
	Transform(const glm::vec3& t, const glm::quat& r, const glm::vec3& s)
	: translation(t), rotation(r), scale(s)
	{}
	
	// Builds and returns an identity transform
	static Transform identity() {
		return Transform(glm::vec3(0.0f), glm::identity<glm::quat>(), glm::vec3(1.0f));
	}
	
	// Combine transformations (translation * rotation * scale) to a single matrix
	glm::mat4 to_matrix() const {
		glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), translation);
		glm::mat4 rotation_matrix = glm::mat4_cast(rotation);  // Convert quaternion to matrix
		glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale);
		return translation_matrix * rotation_matrix * scale_matrix;
	}
};
