#include "scene_layer.h"
#include "imgui_helper.h"

#include "scene/scene.hpp"
#include "graphics/opengl/framebuffer.h"
#include "glcpp/camera.h"
#include "entity/entity.h"
#include <util/utility.h>
#include <entity/components/renderable/mesh_component.h>
#include <entity/components/renderable/armature_component.h>
#include <entity/components/transform_component.h>

#include <memory>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <icons/icons.h>

namespace{

ImVec2 GetMousePosClampedToWindowWithOffsets() {
	ImVec2 mousePos = ImGui::GetMousePos();
	
	// Assume you can obtain your window's position and size.
	ImVec2 windowPos = ImGui::GetWindowPos(); // Top-left corner of the window.
	ImVec2 windowSize = ImGui::GetWindowSize(); // Total size of the window.
	
	// Translate mouse position back to window coordinates.
	mousePos.x -= windowPos.x;
	mousePos.y -= windowPos.y;
	
	return mousePos;
}
}

namespace ui
{
SceneLayer::SceneLayer()
: width_(852),
height_(480),
is_hovered_(false),
is_bone_picking_mode_(true),
is_univ_mode_(false),
is_rotate_mode_(false),
is_scale_mode_(false),
is_translate_mode_(false),
scene_window_right_(0.0f),
scene_window_top_(0.0f),
scene_pos_(0.0f, 0.0f),
current_gizmo_mode_(ImGuizmo::LOCAL),
current_gizmo_operation_(ImGuizmo::OPERATION::NONE)
{
}
// TODO: delete hard coding(scene_cursor_y_ + height_ + 16.0f, 16.0f is font size)
void SceneLayer::draw(const char *title, Scene *scene, UiContext &ui_context)
{
	static ImGuiWindowFlags sceneWindowFlags = 0;
	
	sceneWindowFlags |= ImGuiWindowFlags_NoScrollbar |
	ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollWithMouse;
	
	is_hovered_ = false;
	// add rendered texture to ImGUI scene window
	float width = (float)scene->get_width();
	float height = (float)scene->get_height();
	// draw scene window
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
	
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
	
	ImGui::SetNextWindowSize({width_, height_});

	if (ImGui::Begin(title, 0, sceneWindowFlags))
	{
		scene_pos_ = ImGui::GetWindowPos();
		scene_cursor_y_ = ImGui::GetCursorPosY();
		
		auto framebufferWidth = scene->get_mutable_framebuffer()->get_width();
		auto framebufferHeight = scene->get_mutable_framebuffer()->get_height();
		
		float ratioX = framebufferWidth / scene->get_width();
		float ratioY = framebufferHeight / scene->get_height();
		
		ImVec2 mouse_pos = GetMousePosClampedToWindowWithOffsets();
		int x = static_cast<int>(mouse_pos.x * ratioX);
		int y = static_cast<int>(mouse_pos.y * ratioY);
		// scene->
		ImGuiWindow *window = ImGui::GetCurrentWindow();
		is_hovered_ = !ImGuizmo::IsUsing() && ImGui::IsWindowFocused() && ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(window->InnerRect.Min, window->InnerRect.Max);
		
		is_focused_ = ImGui::IsWindowFocused();
		
		ui_context.scene.is_focused = is_focused_;
		
		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(window->InnerRect.Min, window->InnerRect.Max) && !ui_context.menu.is_dialog_open)
		{
			anim::LOG("double click" + std::to_string(x) + " " + std::to_string(y));
			ui_context.scene.x = x;
			ui_context.scene.y = y;
		}
		
		if(ImGui::IsMouseDown(ImGuiMouseButton_Left)){
			if(ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(window->InnerRect.Min, window->InnerRect.Max)){
				is_hovered_ = true;
			}
		}
		
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(window->InnerRect.Min, window->InnerRect.Max) && !ui_context.menu.is_dialog_open /*&& !is_bone_picking_mode_*/)
		{
			ui_context.scene.x = x;
			ui_context.scene.y = y;
		}
		
		// draw scene framebuffer
		ImGui::Image(reinterpret_cast<void *>(static_cast<intptr_t>(scene->get_mutable_framebuffer()->get_color_attachment())), ImVec2{width_, height_}, ImVec2{0, 1}, ImVec2{1, 0});
		
		draw_gizmo(scene, ui_context);
	}
	draw_mode_window(ui_context);
	
	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	
}

// TODO: delete hard coding(scene_cursor_y_ + 16.0f, 16.0f is font size)
void SceneLayer::draw_gizmo(Scene *scene, UiContext &ui_context)
{
	bool useSnap = true;
	float snap_value = 0.1f;
	float snap[3] = {snap_value, snap_value, snap_value};
	
	ImGuiIO &io = ImGui::GetIO();
	scene_window_right_ = io.DisplaySize.x;
	scene_window_top_ = 0;
	// imguizmo setting
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(scene_pos_.x, (scene_cursor_y_ + 16.0f), width_, height_);
	scene_window_right_ = scene_pos_.x + width_;
	scene_window_top_ = scene_pos_.y;
	
	// scene camera
	auto &entity = scene->get_mutable_ref_camera();
	
	auto camera = entity->get_component<anim::CameraComponent>();
	
	float *cameraView = const_cast<float *>(glm::value_ptr(camera->get_view()));
	float *cameraProjection = const_cast<float *>(glm::value_ptr(camera->get_projection()));
	
	auto selected_entity = scene->get_mutable_selected_entity();
	
	if (selected_entity && current_gizmo_operation_ != ImGuizmo::OPERATION::NONE)
	{
		ui_context.scene.is_gizmo_mode = true;
		auto transform = selected_entity->get_world_transformation();
		glm::mat4 object_matrix = transform;
		float *object_mat_ptr = static_cast<float *>(glm::value_ptr(object_matrix));
		
		ImGuizmo::Manipulate(cameraView,
							 cameraProjection,
							 current_gizmo_operation_,
							 current_gizmo_mode_,
							 object_mat_ptr,
							 NULL,
							 useSnap ? &snap[0] : NULL);
		
		if (ImGuizmo::IsUsing())
		{
			anim::TransformComponent newTransform;
			
			newTransform.set_transform(glm::inverse(transform) * glm::make_mat4(object_mat_ptr));

			ui_context.entity.new_transform = newTransform;
			auto [t, r, s] = anim::DecomposeTransform(ui_context.entity.new_transform.get_mat4());
			
			newTransform.set_transform(selected_entity->get_local() * ui_context.entity.new_transform.get_mat4());
			
			ui_context.entity.new_transform = newTransform;

			ui_context.entity.gizmo_operation = current_gizmo_operation_;
			ui_context.entity.is_changed_transform = true;
			ui_context.entity.is_manipulated = true;
			ui_context.timeline.is_stop = true;
			is_hovered_ = false;
		}
	} else {
		ui_context.scene.is_gizmo_mode = false;
	}
	ImGuizmo::ViewManipulate(cameraView, 8.0f, ImVec2{scene_window_right_ - 128.0f, scene_window_top_ + 16.0f}, ImVec2{128, 128}, ImU32{0x00000000});
}

// TODO: delete hard coding(mode_window_pos)
void SceneLayer::draw_mode_window(UiContext &ui_context)
{
	ImGuiIO &io = ImGui::GetIO();
	ImVec2 mode_window_size = {672.0f, 45.0f};
	ImVec2 mode_window_pos = {scene_window_right_ - width_ * 0.5f - mode_window_size.x * 0.5f, scene_cursor_y_ + height_ - mode_window_size.y};
	
	if (height_ - mode_window_size.y < 5.0f || width_ - mode_window_size.x < 5.0f)
	{
		mode_window_pos = {-1000.0f, -1000.0f};
	}
	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
	
	// Define the original mode_window_size
	ImVec2 original_mode_window_size = {672.0f, 45.0f};
	
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
	
	ImGui::SetNextWindowSize(scaled_mode_window_size);
	
	ImVec2 btn_size{80.0f, 45.0f};

	if(ui_context.scene.editor_mode == EditorMode::Animation){
		ImGui::SetNextWindowPos(mode_window_pos);
	} else {
		ImGui::SetNextWindowPos({mode_window_pos.x + btn_size.x * 2.6f, mode_window_pos.y});
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, mode_window_size);
	ImGui::PushStyleColor(ImGuiCol_Button, {0.3f, 0.3f, 0.3f, 0.8f});
	ImGui::PushStyleColor(ImGuiCol_Text, {1.0f, 1.0f, 1.0f, 1.0f});
	if (ImGui::BeginChild("Child", mode_window_size, NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus))
	{
		
		if (ImGui::BeginTable("split", 1, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_SizingFixedSame))
		{
			current_gizmo_operation_ = ImGuizmo::OPERATION::NONE;
			ImGui::TableNextColumn();
			
			if(ui_context.scene.editor_mode == EditorMode::Animation){
				ToggleButton(ICON_MD_NEAR_ME, &is_bone_picking_mode_, {btn_size.x * 2, btn_size.y}, &ui_context.scene.is_clicked_picking_mode);
				if (ui_context.scene.is_clicked_picking_mode && !is_bone_picking_mode_)
				{
					ui_context.entity.is_changed_selected_entity = true;
					ui_context.entity.selected_id = -1;
				}
				ui_context.scene.is_bone_picking_mode = is_bone_picking_mode_;
				
				ui_context.scene.was_bone_picking_mode = is_bone_picking_mode_;
			}
			ImGui::SameLine();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.f, 0.f});
			// move
			ImGui::PushFont(io.Fonts->Fonts[ICON_FA]);
			ToggleButton(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT, &is_translate_mode_, btn_size);
			if (is_translate_mode_)
			{
				current_gizmo_operation_ = current_gizmo_operation_ | ImGuizmo::OPERATION::TRANSLATE;
			}
			ImGui::PopFont();
			ImGui::SameLine();
			// rotate
			ToggleButton(ICON_MD_FLIP_CAMERA_ANDROID, &is_rotate_mode_, btn_size);
			if (is_rotate_mode_)
			{
				current_gizmo_operation_ = current_gizmo_operation_ | ImGuizmo::OPERATION::ROTATE;
			}
			ImGui::SameLine();
			// scale
			ToggleButton(ICON_MD_PHOTO_SIZE_SELECT_SMALL, &is_scale_mode_, btn_size);
			if (is_scale_mode_)
			{
				current_gizmo_operation_ = current_gizmo_operation_ | ImGuizmo::OPERATION::SCALE;
			}
			
			ImGui::PopStyleVar();
			ImGui::SameLine();
			
			// view mode
			if(ui_context.scene.editor_mode == EditorMode::Animation){
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.f, 0.f});
				if(ToggleButton(ICON_MD_PERSON, &anim::MeshComponent::isActivate, btn_size, &ui_context.scene.was_mesh_activate)){
					if(!anim::MeshComponent::isActivate){
						ui_context.scene.was_mesh_activate = false;
					}
				}
				ImGui::SameLine();
				ImGui::PushFont(io.Fonts->Fonts[ICON_FA]);
				if(ToggleButton(ICON_FA_BONE, &anim::ArmatureComponent::isActivate, btn_size, &ui_context.scene.was_armature_activate)){
					if(!anim::ArmatureComponent::isActivate){
						ui_context.scene.was_armature_activate = false;
					}
				}
				ImGui::PopFont();
				ImGui::PopStyleVar();
			}
			
			if (is_univ_mode_ && !(is_rotate_mode_ && is_scale_mode_ && is_translate_mode_))
			{
				is_univ_mode_ = false;
				is_rotate_mode_ = !is_rotate_mode_;
				is_scale_mode_ = !is_scale_mode_;
				is_translate_mode_ = !is_translate_mode_;
			}
			
			ImGui::EndTable();
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar();
}

float SceneLayer::get_width()
{
	return width_;
}
float SceneLayer::get_height()
{
	return height_;
}
float SceneLayer::get_aspect()
{
	return width_ / height_;
}
bool SceneLayer::get_is_hovered()
{
	return is_hovered_;
}
bool SceneLayer::get_is_focused()
{
	return is_focused_;
}
}
