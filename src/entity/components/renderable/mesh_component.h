#ifndef ANIM_ENTITY_COMPONENT_MESH_COMPONENT_H
#define ANIM_ENTITY_COMPONENT_MESH_COMPONENT_H

#include "../component.h"

#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace anim
{
    class Mesh;
    class Shader;
    class Entity;
    struct MaterialProperties;
    class MeshComponent : public ComponentBase<MeshComponent>
    {
    public:
        static inline bool isActivate = true;
        static inline bool isWireframe = false;
		bool isDynamic{false};
		bool isInterface{false};
		int32_t selectionColor;
		int32_t uniqueIdentifier;

        ~MeshComponent() = default;
        void update(std::shared_ptr<Entity> entity) override;
		void draw_shadow(std::shared_ptr<Entity> entity, anim::Shader& shader);
		
        void set_meshes(const std::vector<std::shared_ptr<Mesh>> &meshes);
        void set_shader(Shader *shader);

        std::vector<MaterialProperties *> get_mutable_mat();
		
		glm::mat4& get_bindpose();
		void set_bindpose(const glm::mat4& bindpose);
		
		std::shared_ptr<Component> clone() override {
			auto clone = std::make_shared<MeshComponent>();
			
			clone->meshes_ = meshes_;
			clone->bindpose_ = bindpose_;
			
			return clone;
		}
		
		void set_path(const std::string& path){
			_path = path;
		}

		const std::string& get_path() const {
			return _path;
		}
		
		const std::tuple<glm::vec3, glm::vec3> get_dimensions(std::shared_ptr<Entity> entity);
		
		void set_unit_scale(float scale){
			_unit_scale = scale;
		}
		
    private:
        std::vector<std::shared_ptr<Mesh>> meshes_;
        Shader *shader_;
		
		glm::mat4 bindpose_;
		
		std::string _path;
		
		float _unit_scale;
    };
}

#endif
