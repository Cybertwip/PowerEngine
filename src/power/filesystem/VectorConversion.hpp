// VectorConversion.hpp
#pragma once

#include <glm/glm.hpp>
#include "SmallFBX/sfbxTypes.h" 

namespace sfbx {

// Convert glm::vec3 to tvec3<double>
inline tvec3<double> glmToDouble3(const glm::vec3& v) {
	return tvec3<double>{ static_cast<double>(v.x), static_cast<double>(v.y), static_cast<double>(v.z) };
}

// Convert glm::vec4 to tvec3<double> by discarding the w component
inline sfbx::double3 glmToDouble3(const glm::vec4& v) {
	return tvec3<double>{ static_cast<double>(v.x), static_cast<double>(v.y), static_cast<double>(v.z) };
}

// If needed, convert tvec3<double> back to glm::vec3
inline glm::vec3 double3ToGLM(const sfbx::double3& v) {
	return glm::vec3{ static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z) };
}

inline sfbx::double4x4 glmMatToSfbxMat(const glm::mat4 &from)
{
	sfbx::double4x4 to;
	// the a,b,c,d in sfbx is the row ; the 1,2,3,4 is the column
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			to[j][i] = from[j][i];
		}
	}
	
	return to;
}

} // namespace sfbx
