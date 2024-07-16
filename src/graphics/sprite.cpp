
#include "sprite.h"

#include "importer.h"

namespace anim
{
Sprite::Sprite(const std::string& path, const MaterialProperties &mat_properties)
: mat_properties_(mat_properties)
{
	texture_.id = Importer::LoadTextureFromFile(path.c_str()); // Implement LoadTextureFromFile according to your texture loading mechanism
	texture_.path = path;

	build_sprite_geometry();
}

void Sprite::build_sprite_geometry()
{
	// Define the vertices for a quad (two triangles)
	vertices_ = {
		Vertex({-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}),
		Vertex({0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}),
		Vertex({0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}),
		Vertex({-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f})
	};
	
	// Define the indices for the two triangles
	indices_ = {0, 1, 2, 2, 3, 0};
}
}
