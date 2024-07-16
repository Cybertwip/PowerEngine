#ifndef GRID_H
#define GRID_H

#include "../shader.h"
#include "glcpp/camera.h"
#include <vector>
#include <glm/glm.hpp>

namespace anim
{
class Grid
{
public:
	Grid();
	~Grid();
	void draw(Shader &shader, anim::CameraComponent &camera);
	
private:
	void setup_grid_buffers();
	
	GLuint grid_VAO_;
	GLuint grid_VBO_;
	std::vector<glm::vec3> grid_vertices_;
};
}

#endif // GRID_H
