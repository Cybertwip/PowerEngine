#pragma once

#include "../component.h"

#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace anim
{
    class Sprite;
    class Shader;
    class Entity;
    struct MaterialProperties;
    class BillboardComponent : public ComponentBase<BillboardComponent>
    {
    public:
        static inline bool isActivate = true;
        static inline bool isWireframe = false;
		bool isDynamic{false};
		bool isInterface{false};
		int32_t selectionColor;
		int32_t uniqueIdentifier;

        ~BillboardComponent() = default;
        void update(std::shared_ptr<Entity> entity) override;
		void draw_shadow(std::shared_ptr<Entity> entity, anim::Shader& shader);
		
        void set_sprite(std::shared_ptr<Sprite> sprite);
        void set_shader(Shader *shader);

        std::vector<MaterialProperties *> get_mutable_mat();
		
		std::shared_ptr<Component> clone() override {
			auto clone = std::make_shared<BillboardComponent>();
			
			clone->sprite_ = sprite_;
			
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
		std::shared_ptr<Sprite> sprite_;
        Shader *shader_;
		
		std::string _path;
		
		float _unit_scale;
    };
}
