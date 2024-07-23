#pragma once

#include "graphics/drawing/Drawable.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <nanogui/vector.h>
#include <glm/glm.hpp>
#include <array>

class ShaderManager;

class GizmoMesh : public Drawable {
public:
    class Vertex {
    public:
        Vertex();
        Vertex(const glm::vec3 &pos);
    
        void set_position(const glm::vec3 &vec);
        glm::vec3 get_position() const;

    private:
        glm::vec3 mPosition;
    };
    
    struct MeshData {
        std::vector<std::unique_ptr<Vertex>> mVertices;
        std::vector<unsigned int> mIndices;
        glm::vec3 mColor;
    };

    class GizmoMeshShader : public ShaderWrapper {
    public:
        GizmoMeshShader(ShaderManager& shaderManager);
        
        void upload_vertex_data(const std::vector<std::unique_ptr<Vertex>>& vertexData);
    };

public:
    GizmoMesh(MeshData& meshData, GizmoMeshShader& shader);
    
    void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection) override;
    
private:
    MeshData& mMeshData;
    GizmoMeshShader& mShader;
    void initialize_mesh();
};
