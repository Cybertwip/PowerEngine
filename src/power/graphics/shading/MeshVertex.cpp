#include "MeshVertex.hpp"

MeshVertex::MeshVertex() {}
MeshVertex(const glm::vec3 &pos) : mPosition(pos) {
	
}
MeshVertex::MeshVertex(const glm::vec3& pos, const glm::vec2& tex)
: mPosition(pos), mNormal(0.0f), mTexCoords1(tex), mTexCoords2(tex) {
}

void MeshVertex::set_position(const glm::vec3& vec) { mPosition = vec; }

void MeshVertex::set_normal(const glm::vec3& vec) { mNormal = vec; }
void MeshVertex::set_color(const glm::vec4& vec) { mColor = vec; }

void MeshVertex::set_texture_coords1(const glm::vec2& vec) { mTexCoords1 = vec; }

void MeshVertex::set_texture_coords2(const glm::vec2& vec) { mTexCoords2 = vec; }

void MeshVertex::set_material_id(int materiaId) { mMaterialId = materiaId; }

glm::vec3 MeshVertex::get_position() const { return mPosition; }

glm::vec3 MeshVertex::get_normal() const { return mNormal; }

glm::vec4 MeshVertex::get_color() const { return mColor; }

glm::vec2 MeshVertex::get_tex_coords1() const { return mTexCoords1; }

glm::vec2 MeshVertex::get_tex_coords2() const { return mTexCoords2; }

int MeshVertex::get_material_id() const {
	return mMaterialId;
}
