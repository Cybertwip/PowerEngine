#ifndef ANIM_UI_UI_CONTEXT
#define ANIM_UI_UI_CONTEXT

#include <string>
#include <glm/glm.hpp>

#include "imgui.h"
#include "ImGuizmo.h"

#include "components/transform_component.h"

namespace ui
{

// Change animation
struct ComponentContext
{
	int current_animation_idx{-1};
	int new_animation_idx{-1};
	bool is_changed_animation{false};
	bool is_clicked_retargeting{false};
	bool is_clicked_remove_animation{false};
	bool is_add_animation_track{false};
	ComponentContext()
	: current_animation_idx(-1),
	new_animation_idx(-1),
	is_changed_animation(false)
	{
	}
};

// Change animator status (play/pause, fps, current time, start time, end time)
struct TimelineContext
{
	bool is_recording{false};
	bool is_clicked_export_sequence{false};
	bool is_clicked_play_back{false};
	bool is_clicked_play{false};
	bool is_stop{false};
	bool is_current_frame_changed{false};
	int start_frame{0};
	int end_frame{200};
	int current_frame{0};
	bool is_forward{false};
	bool is_backward{false};
	bool is_delete_current_frame{false};
	TimelineContext()
	: is_recording(false), is_clicked_play_back(false), is_clicked_play(false), is_stop(false), is_current_frame_changed(false), start_frame(0), end_frame(200), current_frame(0), is_forward(false), is_backward(false), is_delete_current_frame(false)
	{
	}
};

struct AIContext {
	bool is_clicked_add_physics{false};
	bool is_clicked_new_camera{false};
	bool is_clicked_new_light{false};
	bool is_clicked_new_scene{false};
	bool is_clicked_new_sequence{false};
	bool is_clicked_new_composition{false};
	bool is_clicked_import{false};
	bool is_clicked_ai_prompt{false};
	bool is_widget_dragging{false};
};

// import, export
struct MenuContext
{
	bool is_clicked_import_model{false};
	bool is_clicked_import_animation{false};
	bool is_clicked_import_dir{false};
	bool is_clicked_export_animation{false};
	bool is_clicked_export_all_data{false};
	bool is_dialog_open{false};
	bool is_export_linear_interpolation{true};
	float import_scale{100.0f};
	std::string path{""};
	MenuContext()
	: is_clicked_import_model(false),
	is_clicked_import_animation(false),
	is_clicked_import_dir(false),
	is_clicked_export_animation(false),
	is_clicked_export_all_data(false),
	is_dialog_open(false),
	is_export_linear_interpolation(true),
	import_scale(100.0f)
	{
	}
};

// node change (bone, entity)
struct EntityContext
{
	bool is_changed_selected_entity{false};
	bool is_changed_transform{false};
	bool is_manipulated{false};
	int selected_id{-1};
	anim::TransformComponent new_transform;
	ImGuizmo::OPERATION gizmo_operation;

	EntityContext()
	: is_changed_selected_entity(false),
	is_changed_transform(false),
	is_manipulated(false),
	selected_id(-1)
	{
	}
};

enum class EditorMode {
	Map,
	Composition,
	Animation
};

struct SceneContext
{
	bool is_bone_picking_mode{false};
	bool is_clicked_picking_mode{false};
	bool is_mode_change{false};
	bool is_deserialize_scene{false};
	bool is_gizmo_mode{false};
	bool is_cancel_trigger_resources{false};
	bool is_trigger_resources{false};
	bool is_trigger_open_blueprint{false};
	bool is_trigger_update_keyframe{false};
	
	
	bool was_mesh_activate{true};
	bool was_armature_activate{false};
	bool was_bone_picking_mode{false};
	
	bool is_focused{false};

	EditorMode editor_mode = EditorMode::Map;
	
	int x{0};
	int y{0};
	SceneContext()
	:
	is_bone_picking_mode(false),
	is_clicked_picking_mode(false),
	x(-1),
	y(-1)
	{
	}
};
//
//struct PythonContext
//{
//	static inline float min_visibility{0.0f};
//	static inline float min_detection_confidence{0.5f};
//	static inline int model_complexity{1};
//	static inline bool is_angle_adjustment{false};
//	static inline float fps{24.0f};
//	static inline float factor{0.0f};
//	bool is_clicked_convert_btn{false};
//	std::string save_path;
//	std::string video_path;
//	PythonContext()
//	: is_clicked_convert_btn(false),
//	save_path(""),
//	video_path("")
//	{
//	}
//};

struct UiContext
{
	AIContext ai{};
	MenuContext menu{};
	TimelineContext timeline{};
	ComponentContext component{};
	EntityContext entity{};
	SceneContext scene{};
//	PythonContext python{};
	UiContext()
	: menu(),
	timeline(),
	component(),
	entity(),
	scene()
//	python()
	{
	}
};
}

#endif
