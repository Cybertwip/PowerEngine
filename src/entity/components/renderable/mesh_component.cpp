#include "mesh_component.h"
#include "../../../graphics/shader.h"
#include "../../../graphics/mesh.h"
#include "../../entity.h"

#include "utility.h"

namespace anim
{
void MeshComponent::update(std::shared_ptr<Entity> entity)
{
	if (!isActivate)
	{
		return;
	}
	shader_->use();
	
	auto [t, r, s] = DecomposeTransform(entity->get_world_transformation());
	
	s *= _unit_scale;
	
	auto world = ComposeTransform(t, r, s);
	
	shader_->set_mat4("model", world);
	shader_->set_uint("selectionColor", selectionColor);
	shader_->set_uint("uniqueIdentifier", uniqueIdentifier);
	glEnable(GL_DEPTH_TEST); // Enable depth testing

	for (auto &mesh : meshes_)
	{
		if (entity->get_mutable_root()->is_selected_ || entity->is_selected_){
			mesh->draw_outline(*shader_);
		}
		else
		{
			if(isInterface){
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				mesh->draw(*shader_);
				glDisable(GL_CULL_FACE);
			} else {
				mesh->draw(*shader_);
			}
		}
	}
	
}

void MeshComponent::draw_shadow(std::shared_ptr<Entity> entity, anim::Shader& shader){
	shader.set_mat4("model", entity->get_world_transformation());
	
	for (auto &mesh : meshes_)
	{
		mesh->draw_shadow(shader);
	}
}
void MeshComponent::set_meshes(const std::vector<std::shared_ptr<Mesh>> &meshes)
{
	meshes_ = meshes;
}
void MeshComponent::set_shader(Shader *shader)
{
	shader_ = shader;
}

std::vector<MaterialProperties *> MeshComponent::get_mutable_mat()
{
	std::vector<MaterialProperties *> mats;
	for (auto &mesh : meshes_)
	{
		mats.push_back(&mesh->get_mutable_mat_properties());
	}
	return mats;
}

glm::mat4& MeshComponent::get_bindpose()
{
	return bindpose_;
}

void MeshComponent::set_bindpose(const glm::mat4& bindpose){
	bindpose_ = bindpose;
}

const std::tuple<glm::vec3, glm::vec3> MeshComponent::get_dimensions(std::shared_ptr<Entity> entity) {
	
	glm::vec3 p_min;
	glm::vec3 p_max;
	
	auto p = meshes_[0]->get_vertices()[0].position;
	
	p_min.x = p.x;
	p_min.y = p.y;
	p_min.z = p.z;
	
	p_max.x = p.x;
	p_max.y = p.y;
	p_max.z = p.z;

	// Iterate through all meshes and vertices to find min and max coordinates
	for (const auto& mesh : meshes_) {
		const auto& vertices = mesh->get_vertices();
		
		for (const auto& vertex : vertices) {
			auto p = vertex.position;
			
			p_min.x = std::min(p_min.x, p.x);
			p_min.y = std::min(p_min.y, p.y);
			p_min.z = std::min(p_min.z, p.z);
			
			p_max.x = std::max(p_max.x, p.x);
			p_max.y = std::max(p_max.y, p.y);
			p_max.z = std::max(p_max.z, p.z);
		}
	}
	
	return std::make_tuple(p_min * _unit_scale, p_max * _unit_scale);
}

}

