#include "timeline_layer.h"
#include "text_edit_layer.h"
#include "scene/scene.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_neo_sequencer.h>
#include <icons/icons.h>
#include <ImGuizmo.h>
#include <ImSequencer.h>
#include <ImCurveEdit.h>

#include <entity/entity.h>
#include <resources/shared_resources.h>
#include <animation/animation.h>
#include <animation/animator.h>
#include <animation/bone.h>
#include <entity/components/blueprint_component.h>
#include <entity/components/pose_component.h>
#include <entity/components/animation_component.h>
#include <entity/components/serialization_component.h>
#include <entity/components/renderable/light_component.h>
#include "graphics/opengl/framebuffer.h"

#include "blueprint/blueprint.h"

#include "utility.h"

#include <glm/gtc/type_ptr.hpp>

#include "imgui_helper.h"

#include <iostream>
#include <filesystem>
#include <chrono>

namespace {

static std::string frameIndexToTime(int frameIndex, int frameRate) {
	int totalSeconds = frameIndex / frameRate;
	int frames = frameIndex % frameRate;
	
	int hours = totalSeconds / 3600;
	int minutes = (totalSeconds % 3600) / 60;
	int seconds = totalSeconds % 60;
	
	return std::to_string(hours / 10) + std::to_string(hours % 10) + ":" +
	std::to_string(minutes / 10) + std::to_string(minutes % 10) + ":" +
	std::to_string(seconds / 10) + std::to_string(seconds % 10) + ":" +
	std::to_string(frames / 10) + std::to_string(frames % 10);
}
}

using namespace anim;

namespace ui
{

TimelineLayer::TimelineLayer()
{
	BluePrint::NodeProcessor::Setup();
}

TimelineLayer::~TimelineLayer()
{
	BluePrint::NodeProcessor::Teardown();
}


void TimelineLayer::init(Scene *scene){
	hierarchy_entity_ = scene->get_mutable_shared_resources()->get_root_entity();
}


void TimelineLayer::setCinematicSequence(std::shared_ptr<CinematicSequence> sequence){
	animator_->set_active_cinematic_sequence(sequence);
	cinematic_sequence_ = sequence;
	
	auto& sequencer = static_cast<TracksSequencer&>(cinematic_sequence_->get_sequencer());
	
	sequencer.mCompositionStage = CompositionStage::Sequence;
	
	sequencer.mOnKeyFrameSet = [this, &sequencer](int index, int time){
		auto itemId = sequencer.mItems[index]->mId;
		
		auto transformTrack = sequencer.mItems[index]->mTracks[0];
		
		auto entity = resources_->get_entity(itemId);
		
		auto camera = entity->get_component<CameraComponent>();
		auto light = entity->get_component<DirectionalLightComponent>();
		
		int animatorTime = static_cast<int>(animator_->get_current_frame_time());
		
		auto keyframeIt = transformTrack->mKeyFrames.find(time);
		
		if (camera || light || entity->get_component<AnimationComponent>()) {
			if (keyframeIt != transformTrack->mKeyFrames.end()) {
				if (time == animatorTime) {
					if (keyframeIt->second.active) {
						keyframeIt->second.transform = entity->get_local();
					} else {
						// For non-active keyframes, set to previous keyframe's transform, if possible
						if (keyframeIt != transformTrack->mKeyFrames.begin()) {
							auto prevKeyframeIt = std::prev(keyframeIt);
							if (prevKeyframeIt != transformTrack->mKeyFrames.end()) {
								keyframeIt->second.transform = prevKeyframeIt->second.transform;
							}
						} else {
							//@TODO read from the scene's default transform
						}
					}
				} else {
					// For other times, set to previous keyframe's transform, if possible
					if (keyframeIt != transformTrack->mKeyFrames.begin()) {
						auto prevKeyframeIt = std::prev(keyframeIt);
						if (prevKeyframeIt != transformTrack->mKeyFrames.end()) {
							keyframeIt->second.transform = prevKeyframeIt->second.transform;
						}
					} else {
						//@TODO read from the scene's default transform
					}
				}
			} else {
				assert(false);
			}
		}
	};
}


void TimelineLayer::setAnimationSequence(std::shared_ptr<AnimationSequence> sequence){
	animator_->set_active_animation_sequence(sequence);
	animation_sequence_ = sequence;
	
	auto& sequencer = static_cast<AnimationSequencer&>(animation_sequence_->get_sequencer());
	
	sequencer.mOnKeyFrameSet = [this, &sequencer](int index, int time){
		auto itemId = sequencer.mItems[index]->mId;
		
		auto transformTrack = sequencer.mItems[index];
		
		auto entity = resources_->get_entity(itemId);
		
		int animatorTime = static_cast<int>(animator_->get_current_frame_time());
		
		auto keyframeIt = transformTrack->mKeyFrames.find(time);
		if (keyframeIt != transformTrack->mKeyFrames.end()) {
			if (time == animatorTime) {
				if (keyframeIt->second.active) {
					keyframeIt->second.transform = entity->get_local();
				} else {
					// For non-active keyframes, set to previous keyframe's transform, if possible
					if (keyframeIt != transformTrack->mKeyFrames.begin()) {
						auto prevKeyframeIt = std::prev(keyframeIt);
						if (prevKeyframeIt != transformTrack->mKeyFrames.end()) {
							keyframeIt->second.transform = prevKeyframeIt->second.transform;
						}
					} else {
						//@TODO read from the scene's default transform
					}
				}
			} else {
				// For other times, set to previous keyframe's transform, if possible
				if (keyframeIt != transformTrack->mKeyFrames.begin()) {
					auto prevKeyframeIt = std::prev(keyframeIt);
					if (prevKeyframeIt != transformTrack->mKeyFrames.end()) {
						keyframeIt->second.transform = prevKeyframeIt->second.transform;
					}
				} else {
					//@TODO read from the scene's default transform
				}
			}
		} else {
			assert(false);
		}
		
	};
}

void TimelineLayer::setActiveEntity(UiContext &ui_context, std::shared_ptr<Entity> entity){
	
	animation_entity_ = entity;
	auto root = resources_->get_root_entity();
	
	root->add_children(animation_entity_);
	
	root_entity_ = entity->get_mutable_root();
	
	resources_->set_root_entity(entity);
	
	if(animator_->get_current_frame_time() == -1){
		resetAnimatorTime();
	}
	
	if(ui_context.scene.editor_mode == EditorMode::Animation){
		if(animation_entity_->get_animation_tracks()->mTracks.empty()){
			if(auto component = animation_entity_->get_component<ModelSerializationComponent>(); component){
				auto fbxPath = component->get_model_path();
				
				auto animationsPath = std::filesystem::path("Animations");
				auto modelPath = std::filesystem::path(fbxPath);
				
				auto path = std::filesystem::absolute(animationsPath / modelPath);
				
				auto resources = resources_;
				
				resources->import_model(path.string().c_str());
				
				auto& animationSet = resources->getAnimationSet(path.string().c_str());
				
				resources->add_animations(animationSet.animations);
				
				auto animationName = path.replace_extension("").filename().string();
				
				auto subSequencer = animation_entity_->get_animation_tracks();
				
				const auto &animations = animationSet.animations;
				
				for(auto& animation : animations){
					bool existing = false;
					
					for(auto& item : subSequencer->mTracks){
						if(item->mType == ItemType::Transform){
							continue;
						}
						if(item->mId == animation->get_id()){
							existing = true;
							break;
						}
					}
					
					if(!existing){
						subSequencer->mTracks.push_back(std::make_shared<SequenceItem>(animation->get_id(), animationName, ItemType::Animation, 0, (int)animation->get_duration()));
					}
				}
			}
		}
	}
	
}

void TimelineLayer::resetAnimatorTime(){
	animator_->set_current_time(0);
}

void TimelineLayer::disable_camera_from_sequencer(std::shared_ptr<anim::Entity> entity){
	if(!entity){
		return;
	}
	if(cinematic_sequence_){
		auto& sequencer = static_cast<TracksSequencer&>(cinematic_sequence_->get_sequencer());
		auto& sequencerItems = sequencer.mItems; // Reference to the container for easier access
		for (auto it = sequencerItems.begin(); it != sequencerItems.end(); ++it) {
			auto track = *it;
			if(track->mType == TrackType::Camera){
				if (track->mId == entity->get_id()) {
					
					if(sequencer.mSelectedEntry != -1){
						int id = sequencer.mItems[sequencer.mSelectedEntry]->mId;
						
						if(track->mId == id){
							sequencer.mSelectedEntry = -1;
						}
					}
					track->mMissing = true;
				}
			}
		}
	}
}

void TimelineLayer::disable_entity_from_sequencer(std::shared_ptr<Entity> entity){
	if(!entity){
		return;
	}
	if(cinematic_sequence_){
		auto& sequencer = static_cast<TracksSequencer&>(cinematic_sequence_->get_sequencer());
		auto& sequencerItems = sequencer.mItems; // Reference to the container for easier access
		for (auto it = sequencerItems.begin(); it != sequencerItems.end(); ++it) {
			auto track = *it;
			if(track->mType == TrackType::Actor){
				if (track->mId == entity->get_id()) {
					
					if(sequencer.mSelectedEntry != -1){
						int id = sequencer.mItems[sequencer.mSelectedEntry]->mId;
						
						if(track->mId == id){
							sequencer.mSelectedEntry = -1;
						}
					}
					track->mMissing = true;
				}
			}
		}
	}
}

void TimelineLayer::remove_entity_from_sequencer(std::shared_ptr<Entity> entity){
	if(!entity){
		return;
	}
	if(cinematic_sequence_){
		auto& sequencer = static_cast<TracksSequencer&>(cinematic_sequence_->get_sequencer());
		auto& sequencerItems = sequencer.mItems; // Reference to the container for easier access
		for (auto it = sequencerItems.begin(); it != sequencerItems.end(); ) {
			auto track = *it;
			if(track->mType == TrackType::Actor){
				if (track->mId == entity->get_id()) {
					
					if(sequencer.mSelectedEntry != -1){
						int id = sequencer.mItems[sequencer.mSelectedEntry]->mId;
						
						if(track->mId == id){
							sequencer.mSelectedEntry = -1;
						}
					}
					it = sequencerItems.erase(it); // Erase and get new valid iterator
				} else {
					++it; // Only increment iterator if not erasing
				}
			} else {
				++it; // Only increment iterator if not erasing
			}
			
		}
	}
}


void TimelineLayer::remove_camera_from_sequencer(std::shared_ptr<anim::Entity> entity){
	if(!entity){
		return;
	}
	if(cinematic_sequence_){
		auto& sequencer = static_cast<TracksSequencer&>(cinematic_sequence_->get_sequencer());
		auto& sequencerItems = sequencer.mItems; // Reference to the container for easier access
		for (auto it = sequencerItems.begin(); it != sequencerItems.end(); ) {
			auto track = *it;
			if(track->mType == TrackType::Camera){
				if (track->mId == entity->get_id()) {
					
					if(sequencer.mSelectedEntry != -1){
						int id = sequencer.mItems[sequencer.mSelectedEntry]->mId;
						
						if(track->mId == id){
							sequencer.mSelectedEntry = -1;
						}
					}
					it = sequencerItems.erase(it); // Erase and get new valid iterator
				} else {
					++it; // Only increment iterator if not erasing
				}
			} else {
				++it; // Only increment iterator if not erasing
			}
			
		}
		
	}
}


void TimelineLayer::remove_light_from_sequencer(std::shared_ptr<anim::Entity> entity){
	if(!entity){
		return;
	}
	if(cinematic_sequence_){
		auto& sequencer = static_cast<TracksSequencer&>(cinematic_sequence_->get_sequencer());
		auto& sequencerItems = sequencer.mItems; // Reference to the container for easier access
		for (auto it = sequencerItems.begin(); it != sequencerItems.end(); ) {
			auto track = *it;
			if(track->mType == TrackType::Light){
				if (track->mId == entity->get_id()) {
					
					if(sequencer.mSelectedEntry != -1){
						int id = sequencer.mItems[sequencer.mSelectedEntry]->mId;
						
						if(track->mId == id){
							sequencer.mSelectedEntry = -1;
						}
					}
					it = sequencerItems.erase(it); // Erase and get new valid iterator
				} else {
					++it; // Only increment iterator if not erasing
				}
			} else {
				++it; // Only increment iterator if not erasing
			}
			
		}
	}
}

void TimelineLayer::disable_light_from_sequencer(std::shared_ptr<anim::Entity> entity){
	if(!entity){
		return;
	}
	if(cinematic_sequence_){
		auto& sequencer = static_cast<TracksSequencer&>(cinematic_sequence_->get_sequencer());
		auto& sequencerItems = sequencer.mItems; // Reference to the container for easier access
		for (auto it = sequencerItems.begin(); it != sequencerItems.end(); ++it) {
			auto track = *it;
			if(track->mType == TrackType::Light){
				if (track->mId == entity->get_id()) {
					
					if(sequencer.mSelectedEntry != -1){
						int id = sequencer.mItems[sequencer.mSelectedEntry]->mId;
						
						if(track->mId == id){
							sequencer.mSelectedEntry = -1;
						}
					}
					track->mMissing = true;
				}
			}
		}
	}
}


void TimelineLayer::serialize_timeline(){
	if(cinematic_sequence_){
		cinematic_sequence_->serialize();
	}
}

void TimelineLayer::deserialize_timeline(){
	if(cinematic_sequence_){
		cinematic_sequence_->deserialize();
		setCinematicSequence(std::dynamic_pointer_cast<CinematicSequence>(cinematic_sequence_));
	}
}

void TimelineLayer::clearActiveEntity(){
	if(root_entity_){
		if(animation_entity_){
			if(animation_entity_->get_mutable_parent()){
				animation_entity_->get_mutable_parent()->removeChild(animation_entity_);
				resources_->remove_entity(animation_entity_->get_id());
			}
			animation_entity_->set_is_selected(false);
			scene_->set_selected_entity(nullptr);
		}
	}
	
	root_entity_ = nullptr;
	
	resources_->set_root_entity(hierarchy_entity_);
	
}

void TimelineLayer::draw(Scene *scene, UiContext &ui_context)
{
	init_context(ui_context, scene);
	
	
	if(ui_context.scene.editor_mode == EditorMode::Map){
		ImGuiWindowFlags window_flags = 0;
		
		window_flags |= ImGuiWindowFlags_NoTitleBar;
		window_flags |=
		ImGuiWindowFlags_NoMove;
		
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		
		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		
		ImVec2 viewportPanelSize = viewport->WorkSize;
		
		float height = (float)scene->get_height() - 128;
		
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
		
		float scrollAnimationEndPos = viewportPanelSize.y - scaled_mode_window_size.y - 48.0f; // End scroll position (adjust as needed)
		
		ImGui::SetNextWindowPos({0, scrollAnimationEndPos});
		ImGui::SetNextWindowSize(scaled_mode_window_size);
		
		ImGui::Begin(ICON_MD_SCHEDULE " Playback", 0, window_flags | ImGuiWindowFlags_AlwaysAutoResize);
		
		if(ui_context.scene.is_trigger_open_blueprint){
			_blueprint_entity = scene->get_mutable_selected_entity();
		}
		
		if(_blueprint_entity){
			auto blueprint = _blueprint_entity->get_component<BlueprintComponent>();
			
			if(blueprint){
				blueprint->get_processor().OnFrame(0.016f);
			} else {
				
				ImVec2 windowSize = ImGui::GetWindowSize();
				ImVec2 textBounds = ImGui::CalcTextSize("DOUBLE CLICK TO CREATE BLUEPRINT");
				ImVec2 textPos = ImVec2((windowSize.x - textBounds.x * 1.35f) * 0.5f, (windowSize.y - textBounds.y) * 0.5f);
				ImGui::SetCursorPos(textPos);
				ImGui::Text("DOUBLE CLICK TO CREATE BLUEPRINT");
				
				if(ImGui::IsWindowFocused() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)){
					auto blueprint = _blueprint_entity->add_component<BlueprintComponent>();
					blueprint->init(*scene);
				}
			}
		} else {
			auto selected = scene->get_mutable_selected_entity();
			if(selected){
				_blueprint_entity = selected;
			}
		}
		
		if(ui_context.scene.editor_mode == EditorMode::Map){
			
			auto children = resources_->get_root_entity()->get_children_recursive();
			
			children.push_back(resources_->get_root_entity());
			for(auto child : children){
				if(auto blueprint = child->get_component<BlueprintComponent>(); blueprint){
					auto& blueprintProcessor = blueprint->get_processor();
					
					if(BluePrint::NodeProcessor::Evaluating()){
						if(animator_){
							animator_->set_mode(AnimatorMode::Map);
						}
						
						if(animator_){
							ui_context.timeline.is_stop = false;
						}
						
						auto& sequenceStack = blueprintProcessor.GetSequenceStack();
						
						for(auto& sequence : sequenceStack){
							
							auto entity = resources_->get_entity(sequence->get_sequence().mEntityId);
							
							if(!entity){
								continue;
							}
							
							auto anim_component = entity->get_component<AnimationComponent>();
							
							const auto &animations = resources_->get_animations();
							
							anim_component->clear_animation_stack();
							
							auto sequencer = sequence->get_sequencer();
							
							std::vector<std::vector<std::shared_ptr<StackedAnimation>>> animationTracksStack;
							for(auto& sequenceItem : sequencer.mItems){
								
								for(auto& track : sequenceItem->mTracks){
									
									if(track->mType == ItemType::Animation){
										animationTracksStack.push_back({});
									}
									
									for(auto& item : track->mSegments){
										if(track->mType == ItemType::Animation){
											const auto& animation = resources_->get_animation(track->mId);
											
											auto& animationStack = animationTracksStack.back();
											
											animationStack.push_back(std::make_shared<StackedAnimation>(animation, item.mFrameStart, item.mFrameEnd, item.mOffset));
											
											// Custom comparator function to sort based on start_time
											auto comparator = [](const std::shared_ptr<StackedAnimation>& a, const std::shared_ptr<StackedAnimation>& b) {
												return a->get_start_time() < b->get_start_time();
											};
											
											
											std::sort(animationStack.begin(), animationStack.end(), comparator);
										}
									}
								}
							}
							
							auto trackComparator = [&](const std::vector<std::shared_ptr<StackedAnimation>>& trackA, const std::vector<std::shared_ptr<StackedAnimation>>& trackB) {
								if (trackA.empty() && trackB.empty()) {
									return false; // Consider them equal if both are empty, doesn't matter which comes first
								}
								if (trackA.empty()) {
									return true; // Non-empty trackB should come first
								}
								if (trackB.empty()) {
									return false; // Non-empty trackA should come first
								}
								
								// Find the earliest start time in trackA
								auto minStartTimeA = std::min_element(trackA.begin(), trackA.end(),
																	  [](const std::shared_ptr<StackedAnimation>& a, const std::shared_ptr<StackedAnimation>& b) {
									return a->get_start_time() < b->get_start_time();
								});
								
								// Find the earliest start time in trackB
								auto minStartTimeB = std::min_element(trackB.begin(), trackB.end(),
																	  [](const std::shared_ptr<StackedAnimation>& a, const std::shared_ptr<StackedAnimation>& b) {
									return a->get_start_time() < b->get_start_time();
								});
								
								// Compare the earliest start times of trackA and trackB
								return (*minStartTimeA)->get_start_time() < (*minStartTimeB)->get_start_time();
							};
							
							// Sort restackedTracks based on the custom comparator
							std::sort(animationTracksStack.begin(), animationTracksStack.end(), trackComparator);
							
							for(auto& animationTrack : animationTracksStack){
								anim_component->stack_track(animationTrack);
							}
						}
						
						animator_->set_runtime_cinematic_sequence_stack(sequenceStack);
					}
				}
			}
		}
		
		
		ImGui::End();
	}
	
	if(ui_context.scene.editor_mode == EditorMode::Composition || ui_context.scene.editor_mode == EditorMode::Animation) {
		
		if(animator_){
			ui_context.timeline.end_frame = animator_->get_end_time();
		}
		
		ImGuiWindowFlags window_flags = 0;
		
		if (is_hovered_zoom_slider_)
		{
			window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
		}
		
		window_flags |= ImGuiWindowFlags_NoTitleBar;
		window_flags |=
		ImGuiWindowFlags_NoMove;
		
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		
		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		
		ImVec2 viewportPanelSize = viewport->WorkSize;
		
		float height = (float)scene->get_height() - 128;
		
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
		
		float scrollAnimationEndPos = viewportPanelSize.y - scaled_mode_window_size.y - 48.0f; // End scroll position (adjust as needed)
		
		ImGui::SetNextWindowPos({0, scrollAnimationEndPos});
		ImGui::SetNextWindowSize(scaled_mode_window_size);
		
		ImGui::Begin(ICON_MD_SCHEDULE " Playback", 0, window_flags | ImGuiWindowFlags_AlwaysAutoResize);
		{
			ImGui::GetCurrentWindow()->BeginOrderWithinContext = 25;
			
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(1.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 0.0f));
			
			if (ui_context.scene.editor_mode == EditorMode::Composition)
			{
				if(cinematic_sequence_){
					auto& sequencer = static_cast<TracksSequencer&>(cinematic_sequence_->get_sequencer());
					
					if(sequencer.mCompositionStage == CompositionStage::SubSequence){
						if(ui_context.entity.is_manipulated){
							
							ui_context.scene.is_trigger_update_keyframe = true;
							
							auto entity = resources_->get_entity(sequencer.mItems[sequencer.mSelectedEntry]->mId);
							
							
							if(entity->get_component<AnimationComponent>()){
								auto component = entity->get_component<AnimationComponent>();
								
								auto& subSequencer = entity->get_animation_tracks();
								
								int currentFrame = static_cast<int>(animator_->get_current_frame_time());
								
								for(auto& item : sequencer.mItems){
									
									if(!entity || item->mType != TrackType::Actor){
										continue;
									}
									
									for(auto track : item->mTracks){
										if(track->mType == ItemType::Transform){
											auto& keyframes = track->mKeyFrames;
											
											auto it = std::find_if(keyframes.begin(), keyframes.end(), [currentFrame](auto& pair){
												if(pair.first == currentFrame){
													return true;
												}
												
												return false;
											});
											
											if(it != keyframes.end()){
												auto transform = entity->get_world_transformation();
												
												keyframes[currentFrame].transform = transform;
											}
											
										}
									}
								}
							}
						} else {
							ui_context.scene.is_trigger_update_keyframe = false;
						}
					}
					
				}
				
				if(ui_context.scene.is_mode_change){
					clearActiveEntity();
				}
				
				ImGui::BeginChild(ICON_MD_TRACK_CHANGES " Tracks", {}, window_flags);
				
				auto &context = ui_context.timeline;
				ImGuiIO &io = ImGui::GetIO();
				
				ImVec2 button_size(40.0f, 0.0f);
				ImVec2 small_button_size(32.0f, 0.0f);
				const float item_spacing = ImGui::GetStyle().ItemSpacing.x;
				float width = ImGui::GetContentRegionAvail().x;
				
				ImGui::PushItemWidth(30.0f);
				
				ImGui::SameLine();
				
				if(cinematic_sequence_){
					auto& sequencer = static_cast<TracksSequencer&>(cinematic_sequence_->get_sequencer());
					
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
					DragIntProperty(ICON_MD_SPEED, sequencer.mFrameMax, 1.0f, 1.0f, 300.0f);
					ImGui::PopStyleColor(3);
					animator_->set_sequencer_end_time(sequencer.mFrameMax);
				}
				
				int currentFrame = 0;
				
				auto current_cursor = ImGui::GetCursorPosX();
				auto next_pos = ImGui::GetWindowWidth() / 2.0f - button_size.x - small_button_size.x - item_spacing / 2.0;
				if (next_pos < current_cursor)
				{
					next_pos = current_cursor;
				}
				
				if(animator_){
					currentFrame = static_cast<int>(animator_->get_current_frame_time());
					animator_->set_mode(AnimatorMode::Sequence);
				}
				
				ImGui::SameLine();
				
				std::string timeFormat = frameIndexToTime(currentFrame, 60);
				
				ImVec2 textBounds = ImGui::CalcTextSize(timeFormat.c_str());
				
				auto cursorPos = ImGui::GetCursorPos();
				auto windowSize = ImGui::GetWindowSize();
				// let's create the sequencer
				static bool expanded = true;
				
				if(cinematic_sequence_){
					ImGui::SetCursorPosX(next_pos + textBounds.x * 0.125f);
					
					ImGui::Text("%s", timeFormat.c_str());
					
					ImGui::NewLine();
					
					ImGui::SetCursorPosX(next_pos);
					
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
					ImGui::PushStyleColor(ImGuiCol_Text, {1.0f, 0.3f, 0.2f, 0.8f});
					{
						ImGui::PopStyleColor();
						ImGui::PushStyleColor(ImGuiCol_Text, {1.0f, 1.0f, 1.0f, 1.0f});
						
						ToggleButton(ICON_KI_CARET_LEFT, &context.is_backward, small_button_size, &context.is_clicked_play_back);
						ImGui::SameLine();
						ToggleButton(ICON_KI_CARET_RIGHT, &context.is_forward, small_button_size, &context.is_clicked_play);
						ImGui::SameLine();
						ToggleButton(ICON_KI_PAUSE, &context.is_stop, small_button_size);
						
						
					}
					ImGui::PopStyleColor();
					ImGui::PopStyleVar();
					
					
					button_size = ImVec2(180.0f, 0.0f);
					
					current_cursor = ImGui::GetCursorPosX();
					next_pos = ImGui::GetWindowWidth() - button_size.x - small_button_size.x - item_spacing / 2.0;
					if (next_pos < current_cursor)
					{
						next_pos = current_cursor;
					}
					
					ImGui::PushItemWidth(130);
					
					ImGui::SameLine(next_pos);
					
				}
				
				// Export
				if(cinematic_sequence_){
					auto& sequencer = static_cast<TracksSequencer&>(cinematic_sequence_->get_sequencer());
					
					ImGuiIO &io = ImGui::GetIO();
					ImGui::PushFont(io.Fonts->Fonts[ICON_FA]);
					
					bool exportEnabled = !sequencer.mItems.empty();
					
					if(!exportEnabled){
						ImGui::PushStyleColor(ImGuiCol_Button, {0.25, 0.25, 0.25f, 1.0f});
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.25, 0.25, 0.25f, 1.0f});
					}
					
					if(resources_->get_is_exporting()){
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.75, 0.25f, 0.25f, 1.0f});
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.75, 0.125f, 0.125f, 1.0f});
						ImGui::PushStyleColor(ImGuiCol_Button, {0.75, 0.125f, 0.125f, 1.0f});
					}
					
					bool wasDisabled = !exportEnabled;
					
					bool isExport = false;
					
					
					std::string exportIcon = ICON_FA_FILE_EXPORT;
					
					if(resources_->get_is_exporting()){
						exportIcon = ICON_FA_CIRCLE_STOP;
					}
					
					ToggleButton(exportIcon.c_str(), &exportEnabled, button_size, &isExport);
					
					if(resources_->get_is_exporting()){
						ImGui::PopStyleColor(3);
					}
					
					if(wasDisabled && isExport){
						ImGui::PopStyleColor(2);
						context.is_clicked_export_sequence = false;
					} else if(wasDisabled){
						ImGui::PopStyleColor(2);
					} else if(isExport){
						context.is_clicked_export_sequence = true;
					}
					
					ImGui::PopFont();
					
					sequencer.mCurrentTime = animator_->get_current_frame_time();
					
					
					for(auto& item : sequencer.mItems){
						
						auto entity  = resources_->get_scene().get_camera(item->mId);
						
						if(!entity || item->mType != TrackType::Camera){
							continue;
						}
						
						for(auto track : item->mTracks){
							if(track->mType == ItemType::Transform){
								
								auto camera = entity->get_component<anim::CameraComponent>();
								
								auto& subSequencer = entity->get_animation_tracks();
								
								auto& keyframes = track->mKeyFrames;
								
								auto it = std::find_if(keyframes.begin(), keyframes.end(), [currentFrame](auto& pair){
									if(pair.first == currentFrame){
										return true;
									}
									
									return false;
								});
								
								if(it != keyframes.end()){
									auto transform = entity->get_world_transformation();
									keyframes[currentFrame].transform = transform;
								}
							}
						}
					}
					
					for(auto& item : sequencer.mItems){
						auto entity  = resources_->get_scene().get_light(item->mId);
						
						if(!entity || item->mType != TrackType::Light){
							continue;
						}
						
						auto component = entity->get_component<anim::DirectionalLightComponent>();
						
						auto& subSequencer = entity->get_animation_tracks();
						
						for(auto track : item->mTracks){
							if(track->mType == ItemType::Transform){
								auto& keyframes = track->mKeyFrames;
								
								auto it = std::find_if(keyframes.begin(), keyframes.end(), [currentFrame](auto& pair){
									if(pair.first == currentFrame){
										return true;
									}
									
									return false;
								});
								
								if(it != keyframes.end()){
									auto transform = entity->get_world_transformation();
									track->mKeyFrames[currentFrame].transform = transform;
								}
							}
						}
					}
				}
				
				ImGui::PopItemWidth();
				
				int sequencerPick = currentFrame;
				
				if(animator_){
					currentFrame = animator_->get_current_frame_time();
				}
				
				if(cinematic_sequence_){
					auto& sequencer = static_cast<TracksSequencer&>(cinematic_sequence_->get_sequencer());
					
					for(auto& sequencerItem : sequencer.mItems){
						auto entity = resources_->get_entity(sequencerItem->mId);
						
						if(!entity || sequencerItem->mType != TrackType::Actor){
							continue;
						}
						
						auto anim_component = entity->get_component<AnimationComponent>();
						
						auto root = entity->get_mutable_root();
						
						if(anim_component){
							const auto &animations = resources_->get_animations();
							
							if(root){
								anim_component->clear_animation_stack();
								
								auto animationTracks = entity->get_animation_tracks();
								
								std::vector<std::vector<std::shared_ptr<StackedAnimation>>> animationTracksStack;
								
								for(auto& track : animationTracks->mTracks){
									
									if(track->mType == ItemType::Animation){
										animationTracksStack.push_back({});
									} else {
										auto& subSequencer = entity->get_animation_tracks();
										
										auto& keyframes = track->mKeyFrames;
										
										auto it = std::find_if(keyframes.begin(), keyframes.end(), [currentFrame](auto& pair){
											if(pair.first == currentFrame){
												return true;
											}
											
											return false;
										});
										
										if(it != keyframes.end()){
											auto transform = entity->get_world_transformation();
											track->mKeyFrames[currentFrame].transform = transform;
										}
										
										continue;
									}
									
									for(auto& item : track->mSegments){
										const auto& animation = animations[track->mId];
										
										auto& animationStack = animationTracksStack.back();
										
										animationStack.push_back(std::make_shared<StackedAnimation>(animation, item.mFrameStart, item.mFrameEnd, item.mOffset));
										
										// Custom comparator function to sort based on start_time
										auto comparator = [](const std::shared_ptr<StackedAnimation>& a, const std::shared_ptr<StackedAnimation>& b) {
											return a->get_start_time() < b->get_start_time();
										};
										
										
										std::sort(animationStack.begin(), animationStack.end(), comparator);
									}
								}
								
								
								auto trackComparator = [&](const std::vector<std::shared_ptr<StackedAnimation>>& trackA, const std::vector<std::shared_ptr<StackedAnimation>>& trackB) {
									if (trackA.empty() && trackB.empty()) {
										return false; // Consider them equal if both are empty, doesn't matter which comes first
									}
									if (trackA.empty()) {
										return true; // Non-empty trackB should come first
									}
									if (trackB.empty()) {
										return false; // Non-empty trackA should come first
									}
									
									// Find the earliest start time in trackA
									auto minStartTimeA = std::min_element(trackA.begin(), trackA.end(),
																		  [](const std::shared_ptr<StackedAnimation>& a, const std::shared_ptr<StackedAnimation>& b) {
										return a->get_start_time() < b->get_start_time();
									});
									
									// Find the earliest start time in trackB
									auto minStartTimeB = std::min_element(trackB.begin(), trackB.end(),
																		  [](const std::shared_ptr<StackedAnimation>& a, const std::shared_ptr<StackedAnimation>& b) {
										return a->get_start_time() < b->get_start_time();
									});
									
									// Compare the earliest start times of trackA and trackB
									return (*minStartTimeA)->get_start_time() < (*minStartTimeB)->get_start_time();
								};
								
								// Sort restackedTracks based on the custom comparator
								std::sort(animationTracksStack.begin(), animationTracksStack.end(), trackComparator);
								
								for(auto& animationTrack : animationTracksStack){
									anim_component->stack_track(animationTrack);
								}
							}
						}
					}
					
					if(sequencer.mCompositionStage == CompositionStage::Sequence){
						Sequencer(&sequencer, &sequencerPick, &expanded, &sequencer.mSelectedEntry, &sequencer.mFirstFrame,  ImSequencer::SEQUENCER_CHANGE_FRAME);
					} else {
						Sequencer(&sequencer, &sequencerPick, &expanded, &sequencer.mSelectedEntry, &sequencer.mFirstFrame,  ImSequencer::SEQUENCER_CHANGE_FRAME | ImSequencer::SEQUENCER_ADD);
					}
					
					
					if(animator_){
						if(currentFrame != sequencerPick){
							animator_->set_current_time(sequencerPick);
						}
					}
					
					
				} else {
					
					context.is_clicked_export_sequence = false;
					
					ImVec2 windowSize = ImGui::GetWindowSize();
					ImVec2 textBounds = ImGui::CalcTextSize("NO SEQUENCE LOADED");
					ImVec2 textPos = ImVec2((windowSize.x - textBounds.x * 1.35f) * 0.5f, (windowSize.y - textBounds.y) * 0.5f);
					ImGui::SetCursorPos(textPos);
					ImGui::Text("NO SEQUENCE LOADED");
					
					if (ImGui::BeginDragDropTarget()) {
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SEQUENCE_PATH")) {
							const char* sequencePath = reinterpret_cast<const char*>(payload->Data);
							
							auto sequence = std::make_shared<anim::CinematicSequence>(*scene, resources_->shared_from_this(), sequencePath);
							
							setCinematicSequence(sequence);
							
							ui_context.scene.editor_mode = ui::EditorMode::Composition;
							
							ui_context.scene.is_trigger_resources = false;
							ui_context.timeline.is_stop = true;
							ui_context.scene.is_mode_change = true;
							
							ImGui::EndDragDropTarget();
						}
						
					}
				}
				
				if(cinematic_sequence_){
					auto& sequencer = static_cast<TracksSequencer&>(cinematic_sequence_->get_sequencer());
					
					if (sequencer.mCompositionStage == CompositionStage::Sequence && !sequencer.OnDragDropFilter) {
						
						if((sequencer.mSequencerType == TracksSequencer::SequencerType::Single && sequencer.mItems.size() == 0) || sequencer.mSequencerType == TracksSequencer::SequencerType::Composition){
							
							if (ImGui::BeginDragDropTarget()) {
								bool gotPayload = false;
								
								const ImGuiPayload* payload = nullptr;
								if ((payload = ImGui::AcceptDragDropPayload("ACTOR")) ||
									(payload = ImGui::AcceptDragDropPayload("CAMERA")) ||
									(payload = ImGui::AcceptDragDropPayload("LIGHT"))) {
									gotPayload = true;
									
									const char* id_data = reinterpret_cast<const char*>(payload->Data);
									int actor_id = std::atoi(id_data);
									std::shared_ptr<Entity> entity = nullptr;
									
									if (std::strcmp(payload->DataType, "ACTOR") == 0) {
										entity = resources_->get_entity(actor_id);
									} else if (std::strcmp(payload->DataType, "CAMERA") == 0) {
										entity = scene->get_camera(actor_id);
									} else if (std::strcmp(payload->DataType, "LIGHT") == 0) {
										entity = scene->get_light(actor_id);
									}
									
									if (entity) {
										bool existing = false;
										
										for (auto& item : sequencer.mItems) {
											if ((item->mType == TrackType::Camera || item->mType == TrackType::Light) &&
												(std::strcmp(payload->DataType, "ACTOR") == 0)) {
												continue;
											}
											if ((item->mType == TrackType::Actor || item->mType == TrackType::Light) &&
												(std::strcmp(payload->DataType, "CAMERA") == 0)) {
												continue;
											}
											if ((item->mType == TrackType::Actor || item->mType == TrackType::Camera) &&
												(std::strcmp(payload->DataType, "LIGHT") == 0)) {
												continue;
											}
											if (item->mId == entity->get_id()) {
												existing = true;
												break;
											}
										}
										
										if (!existing) {
											TrackType trackType = (std::strcmp(payload->DataType, "ACTOR") == 0) ? TrackType::Actor :
											(std::strcmp(payload->DataType, "CAMERA") == 0) ? TrackType::Camera :
											TrackType::Light;
											
											sequencer.mItems.push_back(std::make_shared<TracksContainer>(entity->get_id(), entity->get_name(), trackType, ImSequencer::SEQUENCER_OPTIONS::SEQUENCER_EDIT_NONE));
											
											auto& subSequencer = entity->get_animation_tracks();
											
											subSequencer = sequencer.mItems.back();
											
											subSequencer->mTracks.push_back(std::make_shared<SequenceItem>(entity->get_id(), "Transform", ItemType::Transform, 0, sequencer.mFrameMax));
										}
									}
								}
								
								if (gotPayload) {
									ImGui::EndDragDropTarget();
								}
							}
							
						}
						
					}
					else if(sequencer.mCompositionStage == CompositionStage::SubSequence) {
						
						if (ImGui::BeginDragDropTarget()) {
							if(sequencer.mSelectedEntry != -1){
								auto resources = scene->get_mutable_shared_resources();
								
								auto entity = resources->get_entity(sequencer.mItems[sequencer.mSelectedEntry]->mId);
								
								auto& subSequencer = entity->get_animation_tracks();
								
								auto camera = entity->get_component<CameraComponent>();
								auto light = entity->get_component<DirectionalLightComponent>();
								
								if(camera == nullptr && light == nullptr){
									if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBX_ANIMATION_PATH")) {
										const char* fbxPath = reinterpret_cast<const char*>(payload->Data);
										
										resources->import_model(fbxPath);
										
										auto& animationSet = resources->getAnimationSet(fbxPath);
										
										resources->add_animations(animationSet.animations);
										
										auto path = std::filesystem::path(fbxPath);
										
										auto animationName = path.replace_extension("").filename().string();
										
										
										auto& sequencer = cinematic_sequence_->get_sequencer();
										
										const auto &animations = animationSet.animations;
										
										for(auto& animation : animations){
											bool existing = false;
											
											for(auto& item : subSequencer->mTracks){
												if(item->mType == ItemType::Transform){
													continue;
												}
												if(item->mId == animation->get_id()){
													existing = true;
													break;
												}
											}
											
											if(!existing){
												subSequencer->mTracks.push_back(std::make_shared<SequenceItem>(animation->get_id(), animationName, ItemType::Animation, 0, (int)animation->get_duration()));
											}
											break;
										}
									}
									ImGui::EndDragDropTarget();
								}
								
							}
						}
					}
				}
				
				ImGui::EndChild();
			}
			
			
			if (ui_context.scene.editor_mode == EditorMode::Animation)
			{
				if(animation_sequence_){
					auto& sequencer = static_cast<AnimationSequencer&>(animation_sequence_->get_sequencer());
					
					if(ui_context.entity.is_manipulated){
						
						ui_context.scene.is_trigger_update_keyframe = true;
						
						auto root = std::dynamic_pointer_cast<AnimationSequence>(animation_sequence_)->get_root();
						
						auto animation = root->get_component<AnimationComponent>();
						
						int currentFrame = static_cast<int>(animator_->get_current_frame_time());
						
						
						if(animation){
							auto &name_bone_map = animation->get_animation()->get_mutable_name_bone_map();
							
							for(auto& track : sequencer.mItems){
								auto& keyframes = track->mKeyFrames;
								
								auto entity = resources_->get_entity(track->mId);
								
								auto it = std::find_if(keyframes.begin(), keyframes.end(), [currentFrame](auto& pair){
									if(pair.first == currentFrame){
										return true;
									}
									
									return false;
								});
								
								if(it != keyframes.end()){
									auto transform = entity->get_local();
									
									auto [t, r, s] = DecomposeTransform(transform);
									
									name_bone_map[entity->get_name()]->push_position(t, currentFrame);
									
									name_bone_map[entity->get_name()]->push_rotation(r, currentFrame);
									
									name_bone_map[entity->get_name()]->push_scale(s, currentFrame);
									
									keyframes[currentFrame].transform = transform;
								}
							}
						}
						
						
					} else {
						ui_context.scene.is_trigger_update_keyframe = false;
					}
					
				}
				
				if(ui_context.scene.is_mode_change){
					resetAnimatorTime();
					if(!animation_entity_){
						root_entity_ = nullptr;
						
						resources_->set_root_entity(nullptr);
					}
				}
				
				if(animation_entity_){
					setActiveEntity(ui_context, animation_entity_);
				}
				
				ImGui::BeginChild(ICON_MD_SCHEDULE " Animation", {}, window_flags);
				
				auto &context = ui_context.timeline;
				ImGuiIO &io = ImGui::GetIO();
				
				ImVec2 button_size(40.0f, 0.0f);
				ImVec2 small_button_size(32.0f, 0.0f);
				const float item_spacing = ImGui::GetStyle().ItemSpacing.x;
				float width = ImGui::GetContentRegionAvail().x;
				
				ImGui::PushItemWidth(30.0f);
				
				ImGui::SameLine();
				
				if(animation_sequence_){
					auto& sequencer = static_cast<AnimationSequencer&>(animation_sequence_->get_sequencer());
					
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
					DragIntProperty(ICON_MD_SPEED, sequencer.mFrameMax, 1.0f, 1.0f, 300.0f);
					ImGui::PopStyleColor(3);
					animator_->set_sequencer_end_time(sequencer.mFrameMax);
				}
				
				int currentFrame = 0;
				
				auto current_cursor = ImGui::GetCursorPosX();
				auto next_pos = ImGui::GetWindowWidth() / 2.0f - button_size.x - small_button_size.x - item_spacing / 2.0;
				if (next_pos < current_cursor)
				{
					next_pos = current_cursor;
				}
				
				if(animator_){
					currentFrame = static_cast<int>(animator_->get_current_frame_time());
					animator_->set_mode(AnimatorMode::Animation);
				}
				
				ImGui::SameLine();
				
				std::string timeFormat = frameIndexToTime(currentFrame, 60);
				
				ImVec2 textBounds = ImGui::CalcTextSize(timeFormat.c_str());
				
				auto cursorPos = ImGui::GetCursorPos();
				auto windowSize = ImGui::GetWindowSize();
				// let's create the sequencer
				static bool expanded = true;
				
				if(animation_sequence_){
					ImGui::SetCursorPosX(next_pos + textBounds.x * 0.125f);
					
					ImGui::Text("%s", timeFormat.c_str());
					
					ImGui::NewLine();
					
					ImGui::SetCursorPosX(next_pos);
					
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
					ImGui::PushStyleColor(ImGuiCol_Text, {1.0f, 0.3f, 0.2f, 0.8f});
					{
						ImGui::PopStyleColor();
						ImGui::PushStyleColor(ImGuiCol_Text, {1.0f, 1.0f, 1.0f, 1.0f});
						
						//						ToggleButton(ICON_KI_REC, &context.is_recording, small_button_size);
						//						is_recording_ = context.is_recording;
						
						//						ImGui::SameLine();
						
						ToggleButton(ICON_KI_CARET_LEFT, &context.is_backward, small_button_size, &context.is_clicked_play_back);
						ImGui::SameLine();
						ToggleButton(ICON_KI_CARET_RIGHT, &context.is_forward, small_button_size, &context.is_clicked_play);
						ImGui::SameLine();
						ToggleButton(ICON_KI_PAUSE, &context.is_stop, small_button_size);
					}
					ImGui::PopStyleColor();
					ImGui::PopStyleVar();
					
					
					button_size = ImVec2(180.0f, 0.0f);
					
					current_cursor = ImGui::GetCursorPosX();
					next_pos = ImGui::GetWindowWidth() - button_size.x - small_button_size.x - item_spacing / 2.0;
					if (next_pos < current_cursor)
					{
						next_pos = current_cursor;
					}
					
					ImGui::PushItemWidth(130);
					
					ImGui::SameLine(next_pos);
					
				}
				
				// Export
				
				//@TODO
				ImGui::PopItemWidth();
				
				ImGui::NewLine();
				
				int sequencerPick = currentFrame;
				
				if(animator_){
					currentFrame = animator_->get_current_frame_time();
				}
				
				if(animation_sequence_){
					auto& sequencer = static_cast<AnimationSequencer&>(animation_sequence_->get_sequencer());
					sequencer.mCurrentTime = animator_->get_current_frame_time();
				}
				
				if(animation_sequence_){
					
					auto& sequencer = static_cast<AnimationSequencer&>(animation_sequence_->get_sequencer());
					
					Sequencer(&sequencer, &sequencerPick, &expanded, &sequencer.mSelectedEntry, &sequencer.mFirstFrame,  ImSequencer::SEQUENCER_CHANGE_FRAME | ImSequencer::SEQUENCER_ADD | ImSequencer::SEQUENCER_SELECT);
					
					if(animator_){
						if(currentFrame != sequencerPick){
							animator_->set_current_time(sequencerPick);
						}
					}
				} else {
					context.is_clicked_export_sequence = false;
					
					ImVec2 windowSize = ImGui::GetWindowSize();
					ImVec2 textBounds = ImGui::CalcTextSize("NO ANIMATION LOADED");
					ImVec2 textPos = ImVec2((windowSize.x - textBounds.x * 1.35f) * 0.5f, (windowSize.y - textBounds.y) * 0.5f);
					ImGui::SetCursorPos(textPos);
					ImGui::Text("NO ANIMATION LOADED");
					
					if (ImGui::BeginDragDropTarget()) {
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBX_ANIMATION_PATH")) {
							const char* sequencePath = reinterpret_cast<const char*>(payload->Data);
							
							clearActiveEntity();
							
							auto resources = resources_;
							
							resources->import_model(sequencePath);
							
							auto& animationSet = resources->getAnimationSet(sequencePath);
							
							resources->add_animations(animationSet.animations);
							auto entity = resources->parse_model(animationSet.model, sequencePath);
							
							auto sequence = std::make_shared<anim::AnimationSequence>(*scene, resources_->shared_from_this(), entity, sequencePath, animationSet.animations[0]->get_id());
							
							setAnimationSequence(sequence);
							
							ui_context.scene.editor_mode = ui::EditorMode::Animation;
							
							ui_context.scene.is_trigger_resources = false;
							ui_context.timeline.is_stop = true;
							ui_context.scene.is_mode_change = true;
							
							ImGui::EndDragDropTarget();
						}
						
					}
				}
				
				ImGui::EndChild();
			}
			
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();
			
		}
		ImGui::End();
		
	}
	
	
}
inline void TimelineLayer::init_context(UiContext &ui_context, Scene *scene)
{
	auto &context = ui_context.timeline;
	scene_ = scene;
	resources_ = scene->get_mutable_shared_resources();
	animator_ = resources_->get_mutable_animator();
	context.start_frame = static_cast<int>(animator_->get_start_time());
	
	context.current_frame = static_cast<int>(animator_->get_current_frame_time());
	context.is_recording = is_recording_;
	if (!context.is_stop)
	{
		context.is_stop = animator_->get_is_stop();
	}
	if (!context.is_stop)
	{
		context.is_forward = (animator_->get_direction() > 0.0f) ? true : false;
		context.is_backward = !context.is_forward;
	}
}

}

