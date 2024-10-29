#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>


class IBone {
public:
	IBone() {
		
	}
	
	virtual ~IBone() = default;
	
	virtual IBone* get_parent() = 0;
	
	virtual std::string get_name() = 0;
	virtual int get_parent_index() = 0;
	virtual const std::vector<int> get_child_indices() const = 0;
	
	virtual void set_translation(const glm::vec3& translation) = 0;
	virtual void set_rotation(const glm::quat& rotation) = 0;
	virtual void set_scale(const glm::vec3& scale) = 0;
	
	virtual glm::vec3 get_translation() const = 0;
	virtual glm::quat get_rotation() const = 0;
	virtual glm::vec3 get_scale() const = 0;
	
	virtual glm::mat4 get_transform_matrix() const = 0;
};
