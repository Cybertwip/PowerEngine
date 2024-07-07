#include "billboard_component.h"
#include "../../../graphics/shader.h"
#include "../../../graphics/sprite.h"
#include "../../entity.h"
#include "utility.h"

#include "glcpp/camera.h"

#include "scene/scene.hpp"

namespace anim
{
void BillboardComponent::update(std::shared_ptr<Entity> entity)
{
	if (!isActivate)
	{
		return;
	}
	auto cameraEntity = sprite_->get_scene().get_active_camera();
	auto camera = cameraEntity->get_component<anim::CameraComponent>();
	
	// Get the positions of the camera and the billboard
	glm::vec3 cameraPos = cameraEntity->get_component<TransformComponent>()->get_translation();
	glm::vec3 billboardPos = entity->get_component<TransformComponent>()->get_translation();
	
	// Calculate the direction from the billboard to the camera
	glm::vec3 direction = glm::normalize(cameraPos - billboardPos);
	
	// Compute the yaw angle (rotation around the Y axis) needed to face the camera
	float yaw = atan2(direction.x, direction.z);
	
	// Create a quaternion for the yaw rotation
	glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));
	

	// Retrieve the transform component
	auto transformComponent = entity->get_component<TransformComponent>();
	auto [t, r, s] = DecomposeTransform(entity->get_world_transformation());

	// Compose the world transformation matrix with the yaw-locked rotation
	auto world = ComposeTransform(t, yawRotation, s);


	// Activate the shader
	shader_->use();
	
	// Scale should include unit scale
	s *= _unit_scale;
	
	// Set the shader uniforms
	shader_->set_mat4("model", world);
	shader_->set_mat4("view", camera->get_view());
	shader_->set_mat4("projection", camera->get_projection());
	shader_->set_uint("selectionColor", selectionColor);
	shader_->set_uint("uniqueIdentifier", uniqueIdentifier);
	
	
	glEnable(GL_DEPTH_TEST); // Enable depth testing
	
	// Check if the entity is selected and draw outline if true
	if (entity->get_mutable_root()->is_selected_ || entity->is_selected_)
	{
		sprite_->draw_outline(*shader_);
	}
	else
	{
		if (isInterface)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			sprite_->draw(*shader_);
			glDisable(GL_CULL_FACE);
		}
		else
		{
			sprite_->draw(*shader_);
		}
	}
}

void BillboardComponent::draw_shadow(std::shared_ptr<Entity> entity, anim::Shader& shader)
{
	
	auto cameraEntity = sprite_->get_scene().get_active_camera();
	
	auto camera = cameraEntity->get_component<anim::CameraComponent>();

	shader.set_mat4("model", entity->get_world_transformation());
	shader.set_mat4("view", camera->get_view());  // Ensure viewMatrix is passed correctly
	shader.set_mat4("projection", camera->get_projection()); // Ensure projectionMatrix is passed correctly
	sprite_->draw_shadow(shader);
}

void BillboardComponent::set_sprite(std::shared_ptr<Sprite> sprite)
{
	sprite_ = sprite;
}

void BillboardComponent::set_shader(Shader* shader)
{
	shader_ = shader;
}

std::vector<MaterialProperties*> BillboardComponent::get_mutable_mat()
{
	std::vector<MaterialProperties*> mats;
	mats.push_back(&sprite_->get_mutable_mat_properties());
	return mats;
}

const std::tuple<glm::vec3, glm::vec3> BillboardComponent::get_dimensions(std::shared_ptr<Entity> entity)
{
	glm::vec3 p_min;
	glm::vec3 p_max;
	
	auto p = sprite_->get_vertices()[0].position;
	
	p_min.x = p.x;
	p_min.y = p.y;
	p_min.z = p.z;
	
	p_max.x = p.x;
	p_max.y = p.y;
	p_max.z = p.z;
	
	// Iterate through all meshes and vertices to find min and max coordinates
	const auto& vertices = sprite_->get_vertices();
	
	for (const auto& vertex : vertices)
	{
		auto p = vertex.position;
		
		p_min.x = std::min(p_min.x, p.x);
		p_min.y = std::min(p_min.y, p.y);
		p_min.z = std::min(p_min.z, p.z);
		
		p_max.x = std::max(p_max.x, p.x);
		p_max.y = std::max(p_max.y, p.y);
		p_max.z = std::max(p_max.z, p.z);
	}
	
	return std::make_tuple(p_min * _unit_scale, p_max * _unit_scale);
}
}
