#include "hierarchy_layer.h"

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "../entity/entity.h"
#include "../scene/scene.hpp"
#include "graphics/opengl/framebuffer.h"

#include "utility.h"

#include <icons/icons.h>
#include <entity/components/pose_component.h>
#include <entity/components/renderable/armature_component.h>
#include <entity/components/renderable/mesh_component.h>
#include <entity/components/renderable/light_component.h>



namespace ui
{
HierarchyLayer::HierarchyLayer() = default;
HierarchyLayer::~HierarchyLayer() = default;
void HierarchyLayer::draw(Scene *scene, UiContext &ui_context)
{
	ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	auto entities = scene->get_mutable_shared_resources()->get_root_entity();
	auto selected_entity = scene->get_mutable_selected_entity();
	if (selected_entity)
	{
		selected_id_ = selected_entity->get_id();
	} else {
		selected_id_ = -2;
	}
	
	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
	
	float height = (float)scene->get_height() / 2.0f + 32;
	
	ImVec2 original_mode_window_size = {viewportPanelSize.x, height};
	
	// Calculate scaling factors for both x and y dimensions
	float scaleX = viewportPanelSize.x / original_mode_window_size.x;
	float scaleY = viewportPanelSize.y / original_mode_window_size.y;
	
	// Choose the smaller scaling factor to maintain aspect ratio
	float scale = std::min(scaleX, scaleY);
	
	// Scale the mode_window_size
	ImVec2 scaled_mode_window_size = {
		original_mode_window_size.x * scale,
		original_mode_window_size.y * scale
	};
	
	float width = (float)scene->get_width();
	
	ImGui::SetNextWindowPos({ width + 5, 15});
	
	ImGui::SetNextWindowSize(scaled_mode_window_size);
	
	static ImGuiWindowFlags window_flags = 0;
	
	window_flags |= ImGuiWindowFlags_NoScrollbar |
	ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
	
	ImGui::Begin("Hierarchy", nullptr, window_flags);
	{
		if(ui_context.scene.editor_mode == ui::EditorMode::Composition ||
		   ui_context.scene.editor_mode == ui::EditorMode::Map){
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBX_ACTOR_PATH")) {
					const char* fbxPath = reinterpret_cast<const char*>(payload->Data);
					
					auto resources = scene->get_mutable_shared_resources();
					
					resources->import_model(fbxPath);
					
					auto& animationSet = resources->getAnimationSet(fbxPath);
					
					resources->add_animations(animationSet.animations);
					auto entity = resources->parse_model(animationSet.model, fbxPath);
					
					auto root = resources->get_root_entity();
					
					std::vector<std::string> existingNames; // Example existing actor names
					std::string prefix = "Actor"; // Prefix for actor names
					
					auto children = root->get_mutable_children();
					
					// Example iteration to get existing names (replace this with your actual iteration logic)
					for (int i = 0; i < children.size(); ++i) {
						std::string actorName = children[i]->get_name();
						existingNames.push_back(actorName);
					}
					
					std::string uniqueActorName = anim::GenerateUniqueActorName(existingNames, prefix);
					
					entity->set_name(uniqueActorName);
					
					root->add_children(entity);
					
					scene->set_selected_entity(entity);
					
					ImGui::EndDragDropTarget();
				}
			}
		}
		
		if (entities)
		{
			if(ui_context.scene.editor_mode == ui::EditorMode::Animation){
				auto selected_node = draw_tree_node(entities, node_flags, scene, ui_context);
				if (selected_node && !(selected_entity && selected_entity->get_id() == selected_node->get_id()))
				{
					ui_context.entity.is_changed_selected_entity = true;
					ui_context.entity.selected_id = selected_node->get_id();
				}
			} else {
				auto selected_node = draw_tree_node(entities, node_flags, scene, ui_context);
				if (selected_node && selected_node != selected_entity)
				{
					ui_context.entity.is_changed_selected_entity = true;
					ui_context.entity.selected_id = selected_node->get_id();
				}
			}
			
		}
	}
	ImGui::End();
}
std::shared_ptr<anim::Entity>    HierarchyLayer::draw_tree_node(std::shared_ptr<anim::Entity> entity_node, const ImGuiTreeNodeFlags &node_flags, Scene *scene, UiContext& ui_context, int depth)
{
	auto selected_flags = node_flags;
	std::shared_ptr<anim::Entity> selected_entity = nullptr;
	auto &childrens = entity_node->get_mutable_children();
	
	if (selected_id_ == entity_node->get_id())
	{
		selected_flags |= ImGuiTreeNodeFlags_Selected;
	}
	
	if (childrens.size() == 0)
	{
		selected_flags |= ImGuiTreeNodeFlags_Leaf;
	}
	if (depth < 2)
	{
		selected_flags |= ImGuiTreeNodeFlags_DefaultOpen;
	}
	std::string label = "";
	
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	
	if (entity_node->get_component<anim::PoseComponent>())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, {0.8f, 0.2f, 0.8f, 1.0f});
		
		ImGui::Text(ICON_MD_SETTINGS_ACCESSIBILITY);
	}
	else if (entity_node->get_component<anim::ArmatureComponent>())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, {0.2f, 0.8f, 0.8f, 1.0f});
		ImGui::Text(ICON_MD_SHARE);
	}
	else if (entity_node->get_component<anim::MeshComponent>())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, {0.8f, 0.8f, 0.2f, 1.0f});
		ImGui::Text(ICON_MD_VIEW_IN_AR);
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Text, {0.1f, 0.1f, 0.1f, 1.0f});
		ImGui::Text(ICON_MD_LIST);
	}
	ImGui::SameLine();
	ImGui::PopStyleColor();
	label += " " + entity_node->get_name() + "##" + std::to_string(entity_node->get_id());
	bool node_open = ImGui::TreeNodeEx(label.c_str(), selected_flags);
	ImGui::PopStyleVar();
	
	if(ImGui::IsItemDoubleClicked() && !ImGui::IsItemToggledOpen()){
		ui_context.scene.is_trigger_open_blueprint = true;
	}
	
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		selected_entity = entity_node;
	}
	
	if (node_open)
	{
		for (size_t i = 0; i < childrens.size(); i++)
		{
			auto child = childrens[i];
			
			selected_flags = 0;
			
			
			if(ui_context.scene.editor_mode == ui::EditorMode::Composition ||
			   ui_context.scene.editor_mode == ui::EditorMode::Map){
				label = "";
				label += " " + child->get_name() + "##" + std::to_string(child->get_id());
				
				if (selected_id_ == child->get_id())
				{
					selected_flags |= ImGuiTreeNodeFlags_Selected;
				}
				
				selected_flags |= ImGuiTreeNodeFlags_Leaf;
				
				bool child_node_open = ImGui::TreeNodeEx(label.c_str(), selected_flags);
				
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
					
					auto payload = std::to_string(child->get_id());
					
					if(child->get_component<anim::CameraComponent>()){
						ImGui::SetDragDropPayload("CAMERA", payload.c_str(), payload.size() + 1, ImGuiCond_Once);
					} else if(child->get_component<anim::DirectionalLightComponent>()){
						ImGui::SetDragDropPayload("LIGHT", payload.c_str(), payload.size() + 1, ImGuiCond_Once);
					} else {
						ImGui::SetDragDropPayload("ACTOR", payload.c_str(), payload.size() + 1, ImGuiCond_Once);
					}
					
					ImGui::Text("%s", child->get_name().c_str());
					
					ImGui::EndDragDropSource();
				}
				
				if(ImGui::IsItemDoubleClicked() && !ImGui::IsItemToggledOpen()){
					ui_context.scene.is_trigger_open_blueprint = true;
				}
				
				if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
				{
					selected_entity = child;
					
					if(auto camera = child->get_component<anim::CameraComponent>(); camera){
						
						scene->set_selected_camera(child);
					}
				}
				
				
				if(child_node_open){
					ImGui::TreePop();
				}
			} else {
				auto selected_child_node = draw_tree_node(child, node_flags, scene, ui_context, depth + 1);
				
				if (selected_child_node != nullptr)
				{
					selected_entity = selected_child_node;
				}
			}
		}
		ImGui::TreePop();
	}
	return selected_entity;
}
}
