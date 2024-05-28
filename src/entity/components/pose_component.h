#ifndef ANIM_ENTITY_COMPONENT_POSE_COMPONENT_H
#define ANIM_ENTITY_COMPONENT_POSE_COMPONENT_H

#include "component.h"
#include "../../resources/model.h"
#include "../../animation/animator.h"
#include "../../graphics/shader.h"

#include <unordered_map>

namespace anim
{
    class AnimationComponent;
    class PoseComponent : public ComponentBase<PoseComponent>
    {
    public:
        PoseComponent();

		std::shared_ptr<Entity> get_root_entity();
        Animator *get_animator();
		std::shared_ptr<AnimationComponent> get_animation_component();

        void set_bone_info_map(std::unordered_map<std::string, BoneInfo> &bone_info_map);
        void set_animator(Animator *animator);
        void set_animation_component(std::shared_ptr<AnimationComponent> animation_component);
        void set_shader(Shader *shader);
		Shader* get_shader();
        void set_armature_root(std::shared_ptr<Entity> armature_root);

        void add_bone(const std::string &name, BoneInfo info);
        void sub_current_bone(const std::string &name);
        void update(std::shared_ptr<Entity> entity) override;
        void add_and_replace_bone(const std::string &name, const glm::mat4 &transform);
		
		std::shared_ptr<Entity> find(int bone_id);
		std::shared_ptr<Entity> find(int bone_id, std::shared_ptr<Entity> entity);

		
		std::shared_ptr<Component> clone() override {
			auto clone = std::make_shared<PoseComponent>();
			clone->bone_info_map_ = bone_info_map_;
			return clone;
		}
		
    private:
        std::unordered_map<std::string, BoneInfo> bone_info_map_{};
        Shader *shader_{nullptr};
        Animator *animator_{nullptr};
		std::shared_ptr<AnimationComponent> animation_component_{nullptr};
		std::shared_ptr<Entity> armature_root_{nullptr};
    };

}

#endif
