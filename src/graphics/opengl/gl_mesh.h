#ifndef ANIM_GRAPHICS_OPENGL_GL_MESH_H
#define ANIM_GRAPHICS_OPENGL_GL_MESH_H

#include "../mesh.h"
#include <memory>

class LightManager;

namespace anim::gl
{
    // TODO: Refactor Create function
    std::unique_ptr<Mesh> CreateBiPyramid(LightManager& lightManager);
    class GLMesh : public anim::Mesh
    {
    public:
        GLMesh(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices, const std::vector<Texture> &textures, const MaterialProperties &mat_properties, LightManager& lightManager);
        GLMesh(const std::vector<Vertex> &vertices, LightManager& lightManager);
        ~GLMesh();
		void draw_mesh(anim::Shader &shader);
		void draw(anim::Shader &shader) override;
		void draw_shadow(anim::Shader &shader) override;
        void draw_outline(anim::Shader &shader) override;
		const std::vector<Vertex>& get_vertices(){
			return vertices_;
		}
		
		void update_vertices(const std::vector<Vertex>& vertices);

    private:
        void init_buffer();
		void draw();

    private:
        unsigned int VAO_, VBO_, EBO_;
		
		LightManager& _lightManager;
    };
}
#endif