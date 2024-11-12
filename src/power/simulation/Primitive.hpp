#pragma once

#include "actors/Actor.hpp"
#include "components/TransformComponent.hpp"

enum PrimitiveShape : int8_t {
	Cube,
	Sphere,
	Cuboid
};

class Primitive {
public:
	Primitive(Actor& actor) : mActor(actor) {
		
	}
	
	
	void set_translation(const glm::vec3& translation) {
		auto& transform = mActor.get_component<TransformComponent>();
		transform.set_translation(translation);
	}
	
	//void set_rotation(const glm::quat& rotation) {
	//	transform.rotation = rotation;
	//	trigger_on_transform_changed();
	//}
	
	//void set_scale(const glm::vec3& scale) {
	//	transform.scale = scale;
	//	trigger_on_transform_changed();
	//}
	
	glm::vec3 get_translation() const {
		auto& transform = mActor.get_component<TransformComponent>();
		return transform.get_translation();
	}
	
	
private:
	Actor& mActor;
};
