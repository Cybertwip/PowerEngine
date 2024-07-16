#include "app.h"

#include <stb/stb_image.h>
#include "glcpp/camera.h"
#include "glcpp/window.h"
#include "scene/scene.hpp"
#include "scene/main_scene.h"
#include "resources/shared_resources.h"
#include "resources/exporter.h"
#include "entity/entity.h"
#include "entity/components/component.h"
#include "entity/components/animation_component.h"
#include <entity/components/blueprint_component.h>
#include "entity/components/renderable/armature_component.h"
#include "entity/components/renderable/mesh_component.h"
#include "entity/components/renderable/light_component.h"
#include "entity/components/renderable/physics_component.h"
#include "animation/animator.h"
#include "util/log.h"
#include "UI/ui_context.h"
#include "resources/exporter.h"

#include "physics/PhysicsWorld.h"

#include "utility.h"

#include "framebuffer.h"

#include "ImGuizmo.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

# include <GLFW/glfw3.h>

#include <thread>

//#include "cpython/py_manager.h"
//#include <pybind11/embed.h>

#include "stb/stb_image_write.h"

namespace{

void takeScreenshot(int width, int height, GLuint textureID, const std::string& filename) {
	// Allocate memory for depth data
	std::vector<float> depthData(width * height); // Assuming depth is stored as float
	
	// Bind the shadow map texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	
	// Read depth data from the texture
	glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depthData.data());
	
	// Convert depth data to RGBA for visualization
	std::vector<unsigned char> pixels(4 * width * height);
	for (int i = 0; i < width * height; ++i) {
		float depth = depthData[i];
		// Convert depth value to grayscale
		unsigned char value = static_cast<unsigned char>(depth * 255.0f);
		// Set RGB channels to grayscale value
		pixels[i * 4] = value;     // Red
		pixels[i * 4 + 1] = value; // Green
		pixels[i * 4 + 2] = value; // Blue
		pixels[i * 4 + 3] = 255;   // Alpha
	}
	
	// Write pixel data to an image file using STB library
	stbi_write_png(filename.c_str(), width, height, 4, pixels.data(), 0);
}

}
namespace fs = std::filesystem;

#ifndef NDEBUG

void error_callback(int code, const char *description)
{
	printf("error %d: %s\n", code, description);
}
#endif
//
void App::init_shared_resources()
{
	shared_resources_ = scenes_[current_scene_idx_]->get_mutable_shared_resources();
}
void App::init_scene(uint32_t width, uint32_t height)
{
	scenes_.push_back(std::make_shared<MainScene>(width, height));
	//scenes_.push_back(std::make_shared<MainScene>(width, height));
	
	
	scenes_[current_scene_idx_]->init_framebuffer(1280, 720);
	//scenes_[ai_scene_idx_]->init_framebuffer(720, 720);

	scenes_[current_scene_idx_]->set_selected_entity(0);
}

void App::update(float deltaTime)
{
	shared_resources_->set_dt(deltaTime);
}
void App::OnStart(){
	stbi_set_flip_vertically_on_load(true);
	
	init_scene(852, 500);
	init_shared_resources();
	history_.reset(new anim::EventHistoryQueue());
	
	init_ui(m_Width, m_Height);
	auto& io = ImGui::GetIO();
	io.IniFilename = "imgui.ini";
}

void App::init_ui(int width, int height)
{
	auto gearPath = std::filesystem::path(DEFAULT_CWD) / "Textures/ui/gear.png";
	auto poweredByPath = std::filesystem::path(DEFAULT_CWD) / "Textures/ui/poweredby.png";
	auto dragAndDropPath = std::filesystem::path(DEFAULT_CWD) / "Textures/ui/draganddrop.png";

	ImTextureID gearTextureId = LoadTexture(gearPath.string().c_str());
	ImTextureID poweredByTextureId = LoadTexture(poweredByPath.string().c_str());
	ImTextureID dragAndDropTextureId = LoadTexture(dragAndDropPath.string().c_str());

	ui_ = std::make_unique<ui::MainLayer>(*scenes_[current_scene_idx_], gearTextureId, poweredByTextureId, dragAndDropTextureId, width, height);
	ui_->init();
	
	ui_->init_timeline(scenes_[current_scene_idx_].get());
}


void App::OnStop(){
	
	
}

void App::OnFrame(float deltaTime){
	ui_->begin();
	
	process_input(deltaTime);
	
	update(deltaTime);
	
	//bool needsRefresh = ui_->draw_ai_widget(scenes_[ai_scene_idx_].get());
	
	//if(needsRefresh){
		//scenes_[current_scene_idx_]->get_mutable_shared_resources()->refresh_directory_node();
	//}
	
	ui_->draw_component_layer(scenes_[current_scene_idx_].get());
	
	ui_->draw_hierarchy_layer(scenes_[current_scene_idx_].get());
	
	ui_->draw_resources(scenes_[current_scene_idx_].get());
	
	ui_->draw_ingame_menu(scenes_[current_scene_idx_].get());
	
	draw_scene(deltaTime);
	
	ui_->draw_timeline(scenes_[current_scene_idx_].get());
	
	ui_->end();
	
	post_update();
	
	post_draw();
	
}
void App::post_update()
{
	process_ai_context();
	process_timeline_context();
	process_menu_context();
	process_scene_context();
	process_component_context();
	//	process_python_context();
}
void App::process_ai_context(){
	auto &ui_context = ui_->get_context();
	auto &ai_context = ui_context.ai;
	
	if(ai_context.is_clicked_new_camera){
		
		shared_resources_->create_camera({125.0f, 50.0f, 0.0f});
	}
	
	if(ai_context.is_clicked_new_light){
		shared_resources_->create_light();
	}
	
	if(ai_context.is_clicked_add_physics){
		auto entity = shared_resources_->get_scene().get_mutable_selected_entity();
		if(entity){
			auto camera = entity->get_component<anim::CameraComponent>();
			auto light = entity->get_component<anim::DirectionalLightComponent>();
			auto physics = entity->get_component<anim::PhysicsComponent>();
			if(!camera && !light && !physics){
								
				auto children = entity->get_children_recursive();
				
				glm::vec3 overall_p_min(std::numeric_limits<float>::max());
				glm::vec3 overall_p_max(std::numeric_limits<float>::lowest());

				for(auto child : children){
					if(auto mesh = child->get_component<anim::MeshComponent>(); mesh){
						auto [mp_min, mp_max] = mesh->get_dimensions(child);
						
						overall_p_min = glm::min(overall_p_min, mp_min);
						overall_p_max = glm::max(overall_p_max, mp_max);
					}
				}
				
				auto extents = overall_p_max - overall_p_min;
				
				auto shader = shared_resources_->get_mutable_shader("ui");
				auto component = entity->add_component<anim::PhysicsComponent>();
				
				component->Initialize(shared_resources_->get_light_manager());
				
				component->set_shader(shader.get());
				
				if(extents != glm::vec3()){
					component->set_extents(extents);
				}
				
				if(entity->get_component<anim::PoseComponent>()){
					component->set_is_dynamic(true);
				} else {
					component->set_is_dynamic(false);
				}
				
				component->set_owner(entity);
				
				shared_resources_->get_scene().get_physics_world().addEntity(entity);
			}
		}
	}
}

void App::process_timeline_context()
{
	auto &ui_context = const_cast<ui::UiContext&>(ui_->get_context());
	auto &time_context = ui_context.timeline;
	auto &entity_context = ui_context.entity;
	auto &scene_context = ui_context.scene;
	auto animator = shared_resources_->get_mutable_animator();
	if (time_context.is_current_frame_changed)
	{
		animator->set_current_time(static_cast<float>(time_context.current_frame));
	}
	
	animator->set_is_stop(time_context.is_stop);
	
	if (time_context.is_clicked_play)
	{
		animator->set_is_stop(false);
		animator->set_direction(false);
	}
	if (time_context.is_clicked_play_back)
	{
		animator->set_is_stop(false);
		animator->set_direction(true);
	}
	if (entity_context.is_changed_selected_entity)
	{
		if(scene_context.editor_mode == ui::EditorMode::Animation){
			
			auto resources = scenes_[current_scene_idx_]->get_mutable_shared_resources();
			
			auto selected_entity = resources->get_entity(entity_context.selected_id);
			
			if(selected_entity){
				auto root_entity = selected_entity->get_mutable_root();
				
				if(selected_entity != root_entity){
					scenes_[current_scene_idx_]->set_selected_entity(selected_entity);
				}
			}
			
		} else {
			scenes_[current_scene_idx_]->set_selected_entity(entity_context.selected_id);
		}
	}
	static uint32_t count = 0u;
	is_manipulated_ = entity_context.is_manipulated;
	if (entity_context.is_changed_transform && time_context.is_recording)
	{
		auto selected_entity = scenes_[current_scene_idx_]->get_mutable_selected_entity();
		auto &before_transform = selected_entity->get_local();
		
		
		if(auto component = selected_entity->get_component<anim::TransformComponent>(); component){
						
			auto gizmo_op = entity_context.gizmo_operation;
			
			if(gizmo_op & ImGuizmo::TRANSLATE){
				auto translation = entity_context.new_transform.get_translation();
				component->set_translation(translation);
			}
			
			if(gizmo_op & ImGuizmo::ROTATE){
				auto rotation = entity_context.new_transform.get_rotation();
				component->set_rotation(rotation);
			}
			
			if(gizmo_op & ImGuizmo::SCALE){
				auto scale = entity_context.new_transform.get_scale();
				component->set_scale(scale);
			}
		}
		

		if (auto component = selected_entity->get_component<anim::ArmatureComponent>(); component)
		{
			auto pose = component->get_pose();
			
			if(pose){
				if(selected_entity != selected_entity->get_mutable_root()){
					pose->add_and_replace_bone(selected_entity->get_name(), selected_entity->get_local());
				}
				
				if (count == 0u)
				{
					history_->push(std::make_unique<anim::BoneChangeEvent>(
																		   scenes_[current_scene_idx_].get(),
																		   scenes_[current_scene_idx_]->get_mutable_shared_resources().get(),
																		   selected_entity->get_id(),
																		   selected_entity->get_mutable_root()->get_component<anim::AnimationComponent>()->get_animation()->get_id(),
																		   before_transform,
																		   pose->get_animator()->get_current_frame_time()));
				}
				
				count++;

			}
			
			
		}
	}
	else
	{
		count = 0u;
	}
	if(time_context.is_delete_current_frame)
	{
		anim::LOG("delete current bone");
		auto selected_entity = scenes_[current_scene_idx_]->get_mutable_selected_entity();
		auto &before_transform = selected_entity->get_local();
		if (auto armature = selected_entity->get_component<anim::ArmatureComponent>(); armature)
		{
			armature->get_pose()->sub_current_bone(armature->get_name());
			history_->push(std::make_unique<anim::BoneChangeEvent>(
																   scenes_[current_scene_idx_].get(),
																   scenes_[current_scene_idx_]->get_mutable_shared_resources().get(),
																   selected_entity->get_id(),
																   selected_entity->get_mutable_root()->get_component<anim::AnimationComponent>()->get_animation()->get_id(),
																   before_transform,
																   armature->get_pose()->get_animator()->get_current_frame_time()));
		}
	}
	
	if(ui_context.timeline.is_clicked_export_sequence){
		if(!shared_resources_->get_is_exporting()){

			anim::MeshComponent::isActivate = false;
			anim::PhysicsComponent::isActivate = false;

			auto children = shared_resources_->get_root_entity()->get_children_recursive();

			std::filesystem::path cwd = std::filesystem::current_path();
			std::filesystem::create_directory(cwd / "Renders");
			std::filesystem::path filepath = cwd / "Renders";
			shared_resources_->start_export_sequence(filepath.string());
			auto animator = shared_resources_->get_mutable_animator();
			
			animator->set_is_stop(true);
			animator->set_current_time(0);
			animator->set_is_stop(false);
		} else {
			animator->set_is_stop(true);
			
			shared_resources_->end_export_sequence(true);
			
			anim::MeshComponent::isActivate = true;
			anim::PhysicsComponent::isActivate = true;
		}
		
	} else {
		if(shared_resources_->get_is_exporting()){
			
			auto frameBuffer = scenes_[current_scene_idx_]->get_mutable_framebuffer();
			shared_resources_->offload_screen(frameBuffer->get_width(), frameBuffer->get_height(), frameBuffer->get_color_attachment());
			
			auto animator = shared_resources_->get_mutable_animator();
			
			if(animator->get_current_frame_time() >= animator->get_sequencer_end_time()){
				animator->set_is_stop(true);
				
				shared_resources_->end_export_sequence(false);
			}
		}
	}
}

void App::process_menu_context()
{
	auto &ui_context = ui_->get_context();
	auto &menu_context = ui_context.menu;
	if (menu_context.is_clicked_import_model)
	{
		shared_resources_->import_model(menu_context.path.c_str(), menu_context.import_scale);
	}
	
	if (menu_context.is_clicked_import_animation)
	{
		//		shared_resources_->import_animation(menu_context.path.c_str(), menu_context.import_scale);
	}
	
	if (menu_context.is_clicked_export_animation)
	{
		shared_resources_->export_animation(scenes_[current_scene_idx_]->get_mutable_selected_entity(), menu_context.path.c_str(), menu_context.is_export_linear_interpolation);
	}
	if (menu_context.is_clicked_import_dir)
	{
		for (const auto &file : std::filesystem::directory_iterator(menu_context.path))
		{
			anim::LOG(file.path().string());
			if (file.path().extension().compare(".fbx") == 0 || file.path().extension().compare(".gltf") == 0)
			{
				anim::LOG("IMPORT");
				shared_resources_->import_model(file.path().string().c_str(), menu_context.import_scale);
			}
		}
	}
	if (menu_context.is_clicked_export_all_data)
	{
		auto entity = scenes_[current_scene_idx_]->get_mutable_selected_entity();
		anim::to_json_all_animation_data(menu_context.path.c_str(), entity, shared_resources_.get());
	}
}

void App::process_scene_context()
{
	auto &ui_context = ui_->get_context();
	auto &scene_context = ui_context.scene;
	is_dialog_open_ = ui_context.menu.is_dialog_open;
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !is_dialog_open_ && scene_context.x != -1 && scene_context.y != -1 && !ImGuizmo::IsUsing() && ui_->is_scene_layer_hovered("scene" + std::to_string(current_scene_idx_ + 1)))
	{
		scenes_[current_scene_idx_]->picking(scene_context.x, scene_context.y, scene_context.editor_mode == ui::EditorMode::Animation, scene_context.is_bone_picking_mode);
	}
	
	if(scene_context.is_deserialize_scene){
		ui_->init_timeline(scenes_[current_scene_idx_].get());
	}
}

void App::process_component_context()
{
	auto &ui_context = const_cast<ui::UiContext&>(ui_->get_context());
	auto &comp_context = ui_context.component;
	if (comp_context.is_changed_animation)
	{
		auto animation_comp = scenes_[current_scene_idx_]->get_mutable_selected_entity()->get_mutable_root()->get_component<anim::AnimationComponent>();
		animation_comp->set_animation(shared_resources_->get_animation(comp_context.new_animation_idx));
	}
	if(comp_context.is_clicked_retargeting) {
		auto pose_comp = scenes_[current_scene_idx_]->get_mutable_selected_entity()->get_mutable_root()->get_component<anim::PoseComponent>();
		
		if(pose_comp->get_animation_component()->get_animation()){
			//			auto retargeter = anim::MixamoRetargeter();
			
			//			auto animation = retargeter.retarget(pose_comp);
			
			//			animation->set_owner(pose_comp->get_root_entity()->get_mutable_root());
			
			//			shared_resources_->add_animation(animation);
			
		}
	}
	
	if(comp_context.is_clicked_remove_animation) {
		
		auto& animations = shared_resources_->get_animations();
		
		if(!animations.empty() && comp_context.current_animation_idx != -1){
			auto pose_comp = scenes_[current_scene_idx_]->get_mutable_selected_entity()->get_mutable_root()->get_component<anim::PoseComponent>();
			
			auto animation = pose_comp->get_animation_component()->get_animation();
			
			animations.erase(std::find_if(animations.begin(), animations.end(), [animation](std::shared_ptr<anim::Animation> resourceAnimation){
				return resourceAnimation == animation;
			}));
			
			pose_comp->get_animation_component()->set_animation(nullptr);
			
			comp_context.current_animation_idx = -1;
			comp_context.is_changed_animation = true;
		}
		
		
	}
}

//
//void App::process_python_context()
//{
//	auto &ui_context = ui_->get_context();
//	auto &py_context = ui_context.python;
//	if (py_context.is_clicked_convert_btn)
//	{
//		auto selected_entity = scenes_[current_scene_idx_]->get_mutable_selected_entity();
//		auto selected_root = (selected_entity) ? selected_entity->get_mutable_root() : nullptr;
//		auto pose_comp = (selected_root) ? selected_root->get_component<anim::PoseComponent>() : nullptr;
//		if (selected_entity && selected_root && pose_comp)
//		{
//			anim::Exporter exporter;
//			std::string model_info = exporter.to_json(pose_comp->get_root_entity());
//			const char *model_info_c = model_info.c_str();
//			auto py = anim::PyManager::get_instance();
//			py->get_mediapipe_pose(anim::MediapipeInfo{py_context.video_path.c_str(),
//				py_context.save_path.c_str(),
//				model_info_c,
//				py_context.min_visibility,
//				py_context.is_angle_adjustment,
//				py_context.model_complexity,
//				py_context.min_detection_confidence,
//				py_context.fps,
//				&py_context.factor});
//			import_model(py_context.save_path.c_str());
//			import_animation(py_context.save_path.c_str());
//		}
//	}
//}

void App::import_model(const char *const path)
{
	shared_resources_->import_model(path, 100.0f);
}

void App::import_animation(const char *const path)
{
	shared_resources_->import_model(path, 100.0f);
}

void App::draw_scene(float dt)
{
	auto &ui_context = const_cast<ui::UiContext&>(ui_->get_context());
	
	size_t size = scenes_.size();
	
	scenes_[current_scene_idx_]->pre_draw(ui_context);
	ui::UiContext ai_context;
	//scenes_[ai_scene_idx_]->pre_draw(ai_context);

	std::string scene_name = std::string("scene") + std::to_string(current_scene_idx_ + 1);

	ui_->draw_scene(scene_name, scenes_[current_scene_idx_].get());
	
//	for (size_t i = 0; i < size; i++)
//	{
//		std::string scene_name = std::string("scene") + std::to_string(i + 1);
//		scenes_[i]->pre_draw(ui_context);
//		ui_->draw_scene(scene_name, scenes_[i].get());
////		if (ui_->is_scene_layer_hovered(scene_name))
////		{
////			//current_scene_idx_ = i;
////		}
//	}
}
//
void App::process_input(float dt)
{
	if (is_dialog_open_ || is_manipulated_)
	{
		return;
	}
	
	auto cameraEntity = scenes_[current_scene_idx_]->get_active_camera();
	
	auto camera = cameraEntity->get_component<anim::CameraComponent>();
	auto app = this;

	if (app->scenes_.size() > app->current_scene_idx_ && !ui_->get_context().scene.is_gizmo_mode && app->ui_->is_scene_layer_hovered("scene" + std::to_string(app->current_scene_idx_ + 1)) && app->ui_->is_scene_layer_focused("scene" + std::to_string(app->current_scene_idx_ + 1))){
		
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_W)))
			camera->process_keyboard(anim::FORWARD, dt);
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_S)))
			camera->process_keyboard(anim::BACKWARD, dt);
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
			camera->process_keyboard(anim::LEFT, dt);
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_D)))
			camera->process_keyboard(anim::RIGHT, dt);
		
	}
//
//	if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftControl)))
//	{
//		anim::LOG("POP::HISTORY");
//		history_->pop();
//		shared_resources_->get_mutable_animator()->set_is_stop(true);
//	}
//	
	if(ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LeftControl)) && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_S))){
		ui_->serialize_timeline();
		
		shared_resources_->serialize("");
		
		//@TODO more serialization etc.
	}
	
	if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)) && app->ui_->is_scene_layer_focused("scene" + std::to_string(app->current_scene_idx_ + 1))){
		auto entity = scenes_[current_scene_idx_]->get_mutable_selected_entity();
		if(entity){
			auto camera = entity->get_component<anim::CameraComponent>();
			auto light = entity->get_component<anim::DirectionalLightComponent>();
			
			int selected_id = entity->get_id();

			if(camera == nullptr && light == nullptr){
				ui_->disable_entity_from_sequencer(entity);
				shared_resources_->remove_entity(selected_id);
			} else if(camera){
				ui_->disable_camera_from_sequencer(entity);
				if(scenes_[current_scene_idx_]->remove_camera(entity)){
					shared_resources_->remove_entity(selected_id);
				}
			} else if(light){
				ui_->disable_light_from_sequencer(entity);
				scenes_[current_scene_idx_]->remove_light(entity);
				shared_resources_->get_light_manager().removeDirectionalLight(entity);
				shared_resources_->remove_entity(selected_id);
			}
			auto children = shared_resources_->get_root_entity()->get_children_recursive();
			
			children.push_back(shared_resources_->get_root_entity());

			for(auto child : children){
				if(auto blueprint = child->get_component<anim::BlueprintComponent>(); blueprint){
					
					auto& blueprintProcessor = blueprint->get_processor();
					
					blueprintProcessor.OnActorRemoved(entity->get_id());

				}
			}
		}
	}
	
	double xposIn = ImGui::GetMousePos().x;
	double yposIn = ImGui::GetMousePos().y;
	if(app->ui_) {
		if (app->is_pressed_ && app->scenes_.size() > app->current_scene_idx_ && !ui_->get_context().scene.is_gizmo_mode && app->ui_->is_scene_layer_hovered("scene" + std::to_string(app->current_scene_idx_ + 1)) && app->ui_->is_scene_layer_focused("scene" + std::to_string(app->current_scene_idx_ + 1)))
		{
			camera->process_mouse_movement((static_cast<float>(yposIn) - app->prev_mouse_.y) / 3.6f, (static_cast<float>(xposIn) - app->prev_mouse_.x) / 3.6f);
		}
	}
	
	app->prev_mouse_.x = app->cur_mouse_.x;
	app->prev_mouse_.y = app->cur_mouse_.y;
	
	app->cur_mouse_.x = xposIn;
	app->cur_mouse_.y = yposIn;
	
	if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && app->ui_ && app->ui_->is_scene_layer_hovered("scene" + std::to_string(app->current_scene_idx_ + 1)))
	{
		app->prev_mouse_.x = app->cur_mouse_.x;
		app->prev_mouse_.y = app->cur_mouse_.y;
		app->is_pressed_ = true;
	}
	else
	{
		app->is_pressed_ = false;
	}
	
	auto& io = ImGui::GetIO();
	
	double xoffset = io.MouseWheelH;
	double yoffset = io.MouseWheel;
	
	
	if(!app->ui_->is_scene_layer_hovered("scene" + std::to_string(app->current_scene_idx_ + 1))){
		app->prev_mouse_.x = 0;
		app->prev_mouse_.y = 0;
	}
	
	if (app->scenes_.size() > app->current_scene_idx_ && app->ui_ && app->ui_->is_scene_layer_hovered("scene" + std::to_string(app->current_scene_idx_ + 1)))
		camera->process_mouse_scroll(yoffset);
	
	
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) &&  app->scenes_.size() > app->current_scene_idx_ && app->ui_ && app->ui_->is_scene_layer_hovered("scene" + std::to_string(app->current_scene_idx_ + 1))){
		auto mouseDragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
		camera->process_mouse_scroll_press(mouseDragDelta.y, mouseDragDelta.x, dt);
		ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
	}
}

void App::post_draw(){
	auto &ui_context = const_cast<ui::UiContext&>(ui_->get_context());

	size_t size = scenes_.size();
	for (size_t i = 0; i < size; i++)
	{
		scenes_[i]->draw(ui_context);
	}
}
