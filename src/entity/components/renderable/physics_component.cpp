#include "physics_component.h"
#include "../../../graphics/shader.h"
#include "../../../graphics/mesh.h"

#include "graphics/opengl/gl_mesh.h"

#include "entity/entity.h"

#include "physics/PhysicsUtils.h"

#include "components/pose_component.h"

#include "utility.h"

namespace anim
{
PhysicsComponent::PhysicsComponent(){
	_extents = glm::vec3(5.0f, 5.0f, 5.0f);
	shape_ = physics::CreateCuboid(_extents);
	// Creating collision boxes for all six faces
	for (int i = 0; i < 4; ++i) {
		_collisionBoxes.push_back(std::make_shared<windy::Rect>(create_collision_box(i)));
	}


}
void PhysicsComponent::set_shader(Shader *shader)
{
	shader_ = shader;
}

void PhysicsComponent::set_extents(const glm::vec3 &extents){
	
	glm::vec3 roundedExtents = glm::ceil(extents);

	auto shape = static_cast<anim::gl::GLMesh*>(shape_.get());
	
	auto vertices = physics::CreateCuboidVertices(roundedExtents);
	
	shape->update_vertices(vertices);
	
	_extents = roundedExtents;
	// Update collision boxes for all faces
	for (int i = 0; i < _collisionBoxes.size(); ++i) {
		*_collisionBoxes[i] = create_collision_box(i);
	}

}

void PhysicsComponent::update(std::shared_ptr<Entity> )
{
	if (!isActivate)
	{
		return;
	}
	
	shader_->use();
	
	auto [t, r, s] = recompute_center();

		
	auto transform = ComposeTransform(t, r, glm::vec3(1.0f, 1.0f, 1.0f));
	
	shader_->set_mat4("model", transform);
	shader_->set_uint("selectionColor", selectionColor);
	shader_->set_uint("uniqueIdentifier", uniqueIdentifier);

	if (_owner->get_mutable_root()->is_selected_ || _owner->is_selected_){
		shape_->draw_outline(*shader_);
	}
	else
	{
		shape_->draw(*shader_);
	}

}

std::tuple<glm::vec3, glm::quat, glm::vec3> PhysicsComponent::recompute_center(){
	
	auto [t, r, s] = DecomposeTransform(_owner->get_world_transformation());
	
	if(_root_motion){
		auto pose = _owner->get_component<PoseComponent>();
		if(pose){
			if(pose->get_root_entity()){
				auto root = pose->get_root_entity();
				auto [wrt, wrr, wrs] = DecomposeTransform(root->get_world_transformation());
				
				t = wrt;
				r = wrr;
				s = wrs;
			}
		}
	}
	
	
	t.y += _extents.y / 2.0f;
	
	t += _offset;

	_center = t;
	
	return {t, r, s};
}

windy::Rect PhysicsComponent::create_collision_box(int faceIndex) {
	windy::Rect collisionBox;
	switch (faceIndex) {
		case 0: // Front face
			collisionBox.origin = _center - glm::ivec3(_extents.x / 2.0f, _extents.y / 2.0f, 0.0f);
			collisionBox.size = glm::vec2(_extents.x, _extents.y);
			break;
		case 1: // Back face
			collisionBox.origin = _center - glm::ivec3(_extents.x / 2.0f, _extents.y / 2.0f, 0.0f);
			collisionBox.size = glm::vec2(_extents.x, _extents.y);
			break;
		case 2: // Left face
			collisionBox.origin = _center - glm::ivec3(0.0f, _extents.y / 2.0f, _extents.z / 2.0f);
			collisionBox.size = glm::vec2(_extents.z, _extents.y);
			break;
		case 3: // Right face
			collisionBox.origin = _center - glm::ivec3(0.0f, _extents.y / 2.0f, _extents.z / 2.0f);
			collisionBox.size = glm::vec2(_extents.z, _extents.y);
			break;
		case 4: // Top face
			collisionBox.origin = _center - glm::ivec3(_extents.x / 2.0f, 0.0f, _extents.z / 2.0f);
			collisionBox.size = glm::vec2(_extents.x, _extents.z);
			break;
		case 5: // Bottom face
			collisionBox.origin = _center - glm::ivec3(_extents.x / 2.0f, 0.0f, _extents.z / 2.0f);
			collisionBox.size = glm::vec2(_extents.x, _extents.z);
			break;
	}
	return collisionBox;
}


std::shared_ptr<windy::Rect> PhysicsComponent::get_collision_box(int index) {
	*_collisionBoxes[index] = create_collision_box(index);
	return _collisionBoxes[index];
}

std::size_t PhysicsComponent::get_collision_size(){
	return _collisionBoxes.size();
}
}
