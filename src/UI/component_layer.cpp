#include "component_layer.h"

#include "scene/scene.hpp"
#include "imgui_helper.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "graphics/opengl/framebuffer.h"

#include "glcpp/camera.h"

#include "utility.h"

#include <entity/entity.h>
#include <entity/components/renderable/mesh_component.h>
#include <entity/components/renderable/light_component.h>
#include <entity/components/renderable/physics_component.h>
#include <entity/components/animation_component.h>
#include <animation/animation.h>

using namespace anim;

namespace ui
{
ComponentLayer::ComponentLayer() = default;

ComponentLayer::~ComponentLayer() = default;

void ComponentLayer::draw(ComponentContext &context, Scene *scene)
{
	std::shared_ptr<Entity> entity = scene->get_mutable_selected_entity();
	SharedResources *resources = scene->get_mutable_shared_resources().get();
	
	//		ImGuiWindowClass window_class;
	//		window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
	//
	//		ImGui::SetNextWindowClass(&window_class);
	
	
	// Define the original mode_window_size
	
	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
	
	float height = (float)scene->get_height() / 2.0f;
	
	ImVec2 original_mode_window_size = {224, height};
	
	// Calculate scaling factors for both x and y dimensions
	float scaleX = viewportPanelSize.x / original_mode_window_size.x;
	float scaleY = viewportPanelSize.y / original_mode_window_size.y;
	
	// Choose the smaller scaling factor to maintain aspect ratio
	float scale = std::min(scaleX, scaleY);
	
	// Scale the mode_window_size
	ImVec2 scaled_mode_window_size = {
		original_mode_window_size.x * scale,
		original_mode_window_size.y
	};
	
	
	float width = (float)scene->get_width();
	
	ImGui::SetNextWindowPos({ width + 5, height + 44});
	
	ImGui::SetNextWindowSize(scaled_mode_window_size);
	
	static ImGuiWindowFlags window_flags = 0;
	
	window_flags |= ImGuiWindowFlags_NoScrollbar |
	ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus;
	if(entity && entity->get_mutable_parent() != nullptr){
		if (ImGui::Begin("Properties", nullptr, window_flags))
		{
			if (entity)
			{
				if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_DefaultOpen))
					
				{
					draw_transform(entity);
				}
				
				
				// TODO mesh and materials editor
				
				//                if (auto root = entity->get_mutable_root(); root)
				//                {
				//					auto animation = root->get_component<AnimationComponent>();
				//
				//                    if (animation != nullptr && ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_Bullet))
				//                    {
				//                        draw_animation(context, resources, root, animation);
				//                        ImGui::Separator();
				//                    }
				//                }
				//                if (auto mesh = entity->get_component<anim::MeshComponent>(); mesh && ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_Bullet))
				//                {
				//                    draw_mesh(mesh);
				//                }
			}
		}
		ImGui::End();
	} else {
		if(entity){
			if(entity == scene->get_mutable_shared_resources()->get_root_entity()){
				
				

				if (ImGui::Begin("Properties", nullptr, window_flags))
				{
					if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_DefaultOpen))
						
					{
						draw_transform(entity);
					}

					if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_DefaultOpen)){
						ImGui::PushItemWidth(24 * scaleX);
						ImGui::ColorEdit3("Background Color", &scene->get_background_color()[0], ImGuiColorEditFlags_NoInputs);
						ImGui::PopItemWidth();
					}
				}
				
				ImGui::End();
			}
		} else {
			auto camera = scene->get_mutable_ref_camera();
			
			if(camera){
				if (ImGui::Begin("Details", nullptr, window_flags))
				{
					if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_DefaultOpen))
						
					{
						draw_transform(camera);
					}
				}
				ImGui::End();
			}
		}
	}
	
}

void ComponentLayer::draw_animation(ComponentContext &context, SharedResources *shared_resource, std::shared_ptr<Entity> entity, const AnimationComponent *animation)
{
	if(animation->get_animation() != nullptr){
		context.current_animation_idx = animation->get_animation()->get_id();
	} else {
		context.current_animation_idx = -1;
		context.is_changed_animation = true;
	}
	int animation_idx = context.current_animation_idx;
	
	auto &resourceAnimations = shared_resource->get_animations();
	
	std::vector<std::shared_ptr<Animation>> animations;
	
	for(auto& resourceAnimation : resourceAnimations){
		
		if(auto component = entity->get_component<AnimationComponent>(); component){
			if(auto animation = component->get_animation()){
				if(resourceAnimation->get_id() == animation->get_id()){
					animations.push_back(resourceAnimation);
				}
			}
		}
	}
	
	const char *names[] = {"Animation"};
	ImGuiStyle &style = ImGui::GetStyle();
	
	float child_w = (ImGui::GetContentRegionAvail().x - 1 * style.ItemSpacing.x);
	if (child_w < 1.0f)
		child_w = 1.0f;
	
	ImGui::PushID("##VerticalScrolling");
	for (int i = 0; i < 1; i++)
	{
		const ImGuiWindowFlags child_flags = 0;
		const ImGuiID child_id = ImGui::GetID((void *)(intptr_t)i);
		const bool child_is_visible = ImGui::BeginChild(child_id, ImVec2(child_w, 200.0f), true, child_flags);

		if (child_is_visible)
		{
			for (auto& animation : animations)
			{
				std::string name = std::to_string(animation->get_id()) + ":" + animation->get_name();
				
				bool is_selected = (animation->get_id() == animation_idx);
				if (ImGui::Selectable(name.c_str(), is_selected))
				{
					animation_idx = animation->get_id();
				}
				
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					context.is_add_animation_track = true;
				}
			}
		}
		
		ImGui::EndChild();
	}
	if(ImGui::Button("+")) {
		context.is_clicked_retargeting = true;
	}
	
	ImGui::SameLine();
	if(ImGui::Button("-")) {
		context.is_clicked_remove_animation = true;
	}
	
	auto animc = const_cast<AnimationComponent *>(animation);
	auto anim = animc->get_animation();
	//        ImGui::Text("duration: %f", anim->get_duration());
	//        ImGui::Text("fps: %f", anim->get_fps());
	if (animation_idx != context.current_animation_idx)
	{
		context.new_animation_idx = animation_idx;
		context.is_changed_animation = true;
	}
	//        ImGui::DragFloat("custom fps", &fps, 1.0f, 1.0f, 144.0f);
	ImGui::PopID();
}

void ComponentLayer::draw_transform(std::shared_ptr<Entity> entity)
{
	if(auto camera = entity->get_component<CameraComponent>(); camera){
		auto component = entity->get_component<TransformComponent>();
				
		if(DragPropertyXYZ("Translation", component->mTranslation)){
			entity->set_local(component->get_mat4());
			camera->update_camera_vectors();
		}

		auto euler = glm::degrees(component->mRotation);
		
		auto pitchRoll = glm::vec3(euler.x, euler.y, euler.z);

		if(DragFPropertyPitchYawRoll("Rotation", &pitchRoll[0])){
			component->mRotation = glm::radians(pitchRoll);
			camera->update_camera_vectors();
		}
		
	} else if(auto light = entity->get_component<DirectionalLightComponent>(); light){
		
		auto transform = entity->get_component<TransformComponent>();
		auto translation = transform->mTranslation;
		
		auto euler = glm::degrees(transform->mRotation);
		
		if(DragPropertyXYZ("Translation", translation)){
			transform->mTranslation = translation;
		}
		
		auto pitchRoll = glm::vec3(euler.x, euler.y, euler.z);
		
		if(DragFPropertyPitchYawRoll("Rotation", &pitchRoll[0])){
			transform->mRotation = glm::radians(glm::vec3(pitchRoll.x, pitchRoll.y, pitchRoll.z));
		}
	} else if(auto transform = entity->get_component<TransformComponent>(); transform){
		auto euler = glm::degrees(transform->mRotation);
		
		auto translation = transform->mTranslation;
		
		if(DragPropertyXYZ("Translation", translation)){
			transform->mTranslation = translation;
		}
		if(DragPropertyXYZ("Rotation", euler)){
			transform->mRotation = glm::radians(euler);
		}
		auto scale = transform->mScale;
		
		if(DragPropertyXYZ("Scale", scale)){
			transform->mScale = scale;
		}
	}
	
	if(auto physics = entity->get_component<PhysicsComponent>(); physics){
		
		if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_DefaultOpen)){

			
			auto offset = physics->get_offset();
			
			auto newOffset = glm::vec3(offset.x, offset.y, offset.z);
			
			if(DragPropertyXYZ("Offset", newOffset, 1.0f)){
				physics->set_offset(newOffset);
			}

			auto extents = physics->get_extents();
			auto newExtents = glm::vec3(extents.x, extents.y, extents.z);
			

			if(DragPropertyXYZ("Size", newExtents, 1.0f)){
				physics->set_extents(newExtents);
			}
			
			ImGui::NewLine();
			
			auto dynamic = physics->is_dynamic();

			if(ImGui::Checkbox("Dynamic", &dynamic)){
				physics->set_is_dynamic(dynamic);
			}
			
			if(auto posable = entity->get_component<PoseComponent>()){

				auto root_motion = physics->get_root_motion();
			
				ImGui::NewLine();
				if(ImGui::Checkbox("Root Motion", &root_motion)){
					physics->set_root_motion(root_motion);
				}
			}

		}
	}
	
	if(auto light = entity->get_component<DirectionalLightComponent>(); light){
		if (ImGui::CollapsingHeader("Light Parameters", ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_DefaultOpen)){
			// Calculate the width for each column based on available content width
			float columnWidth = ImGui::GetContentRegionAvail().x / 3.0f;
			
			ImGui::Columns(3, nullptr, false);
			
			// Set the width for each column
			for (int i = 0; i < 3; ++i)
				ImGui::SetColumnWidth(i, columnWidth);
			
			ImGui::ColorEdit3("Diffuse", &light->get_parameters().diffuse[0], ImGuiColorEditFlags_NoInputs);
			ImGui::NextColumn();
			
			ImGui::ColorEdit3("Ambient", &light->get_parameters().ambient[0], ImGuiColorEditFlags_NoInputs);
			ImGui::NextColumn();
			
			ImGui::ColorEdit3("Specular", &light->get_parameters().specular[0], ImGuiColorEditFlags_NoInputs);
			
			ImGui::Columns(1);
		}
	}
	
	//ImGui::Separator();
}

void ComponentLayer::draw_transform_reset_button(anim::TransformComponent &transform)
{
	if (ImGui::Button("reset"))
	{
		transform.set_translation({0.0f, 0.0f, 0.0f}).set_rotation({0.0f, 0.0f, 0.0f}).set_scale({1.0f, 1.0f, 1.0f});
	}
}
void ComponentLayer::draw_mesh(anim::MeshComponent *mesh)
{
	auto material = mesh->get_mutable_mat();
	int idx = 0;
	for (auto &mat : material)
	{
		ImGui::ColorPicker3(("diffuse " + std::to_string(idx)).c_str(), &mat->diffuse[0]);
		
		idx++;
	}
}
}
