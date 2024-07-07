#pragma once

#include "../sprite.h"
#include <memory>

namespace anim::gl
{
class GLSprite : public anim::Sprite
{
public:
	GLSprite(const std::string& path, const MaterialProperties &mat_properties, Scene& scene);
	~GLSprite();
	void draw(anim::Shader &shader) override;
	void draw_shadow(anim::Shader &shader) override;
	void draw_outline(anim::Shader &shader) override;
	
	void update_vertices(const std::vector<Vertex>& vertices);
	
private:
	void init_buffer();
	void draw_sprite(anim::Shader &shader);
	void draw();
	
	unsigned int VAO_, VBO_, EBO_;
};

}
