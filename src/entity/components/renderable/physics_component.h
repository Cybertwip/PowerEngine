#pragma once

#include "../component.h"

#include "physics/Geometry.h"
#include "physics/GeometryExtensions.h"
#include "physics/PhysicsWorld.h"

#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"

#include <cassert>

class LightManager;

namespace windy{
struct CollisionContact;
}

namespace anim
{
    class Shader;
    class Mesh;
    class Entity;

    class PhysicsComponent : public ComponentBase<PhysicsComponent>
    {
	public:        
		static inline bool isActivate = true;

		int32_t selectionColor;
		int32_t uniqueIdentifier;

		PhysicsComponent();
		
		void Initialize(LightManager& lightManager);

		void set_owner(std::shared_ptr<Entity> owner){
			_owner = owner;
		}

		
		std::tuple<glm::vec3, glm::quat, glm::vec3> recompute_center();
		
		void set_shader(Shader *shader);

		const glm::ivec3& get_extents() const {
			return _extents;
		}

		void set_extents(const glm::vec3& extents);

		const glm::ivec3& get_offset() const {
			return _offset;
		}

		void set_offset(const glm::vec3& offset){
			_offset = offset;
		}

		void set_root_motion(bool root_motion) {
			_root_motion = root_motion;
		}
		
		bool get_root_motion() const {
			return _root_motion;
		}
		
		std::shared_ptr<windy::Rect> get_collision_box(int index);

		std::size_t get_collision_size();

		void update(std::shared_ptr<Entity> entity) override;

		std::shared_ptr<Component> clone() override {
			assert(nullptr && "Not implemented");
			return nullptr;
		}
		
		void recreate_contacts(){
			_contacts.clear();
		}
		
		std::shared_ptr<windy::CollisionContact> get_contact(windy::Contact contact){
			auto it = std::find_if(_contacts.begin(), _contacts.end(), [contact](std::shared_ptr<windy::CollisionContact> collisionContact){
				return collisionContact->type == contact;
			});
			
			if(it != _contacts.end()){
				return *it;
			}
			
			return nullptr;
		}

		std::vector<std::shared_ptr<windy::CollisionContact>>& get_contacts_internal() {
			return _contacts;
		}
		
		glm::vec3& get_speed(){
			return _speed;
		}
		
		bool is_dynamic(){
			return _dynamic;
		}
		
		void set_is_dynamic(bool dynamic){
			_dynamic = dynamic;
			
			_speed = {0.0f, 0.0f, 0.0f};
		}

    private:
		windy::Rect create_collision_box(int faceIndex);

        std::unique_ptr<Mesh> shape_;
        Shader *shader_;
		glm::ivec3 _extents;
		glm::ivec3 _offset;
		glm::ivec3 _center;
		glm::vec3 _speed;

		std::vector<std::shared_ptr<windy::Rect>> _collisionBoxes; // Collision boxes for all faces

		std::vector<std::shared_ptr<windy::CollisionContact>> _contacts;

		bool _dynamic;

		bool _root_motion = false;
		
		std::shared_ptr<Entity> _owner;
    };
}

