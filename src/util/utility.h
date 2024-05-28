#ifndef ANIM_UTIL_UTILITY_H
#define ANIM_UTIL_UTILITY_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <SmallFBX.h>

#include <tuple>

#include <string>
#include <regex>
#include <algorithm>
#include <vector>
#include <iostream> // For debugging
#include <cassert> // For debugging

namespace anim
{
// Helper function to generate a unique numeric-based actor name
inline std::string GenerateUniqueActorName(const std::vector<std::string>& existingNames, const std::string& prefix) {
	int maxNumber = 0; // Track the highest number found
	std::regex actorPattern("^" + prefix + " (\\d+)$"); // Pattern to match actor names with numbers
	
	for (const auto& actorName : existingNames) {
		std::smatch match;
		
		if (std::regex_match(actorName, match, actorPattern) && match.size() == 2) {
			int number = std::stoi(match[1].str());
			maxNumber = std::max(maxNumber, number);
		}
	}
	
	// Generate the new actor name with the next number
	std::string newActorName = prefix + " " + std::to_string(maxNumber + 1);
	return newActorName;
}

inline glm::vec3 SfbxVecToGlmVec(const sfbx::float3 &vec)
{
	return glm::vec3(vec.x, vec.y, vec.z);
}
inline glm::mat4 SfbxMatToGlmMat(const sfbx::float4x4 &from)
{
	glm::mat4 to;
	// the a,b,c,d in sfbx is the row ; the 1,2,3,4 is the column
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			to[j][i] = from[j][i];
		}
	}
	
	return to;
}

// return translate, rotate, scale
inline std::tuple<glm::vec3, glm::quat, glm::vec3> DecomposeTransform(const glm::mat4 &transform)
{
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transform, scale, rotation, translation, skew, perspective);
	return {translation, rotation, scale};
}
inline glm::mat4 ComposeTransform(const glm::vec3 &translate, const glm::quat &rotate, const glm::vec3 &scale)
{
	glm::mat4 transform = glm::mat4(1.0f);
	transform = glm::translate(transform, translate);
	transform = transform * glm::mat4_cast(rotate);
	transform = glm::scale(transform, scale);
	return transform;
}
inline glm::vec3 GetRelativePos(const glm::vec4 &world_pos, const glm::mat4 &world_mat)
{
	glm::vec4 origin = world_mat * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec3 xAxis = glm::normalize(world_mat * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
	glm::vec3 yAxis = glm::normalize(world_mat * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	glm::vec3 zAxis = glm::normalize(world_mat * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
	
	glm::vec4 b = world_pos - origin;
	glm::mat3 A = glm::mat3(xAxis, yAxis, zAxis);
	
	glm::vec3 x = glm::inverse(A) * glm::vec3(b);
	return x;
}
}
#endif
