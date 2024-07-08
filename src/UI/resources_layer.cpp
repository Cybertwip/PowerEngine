#include "resources_layer.h"
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
#include <entity/components/pose_component.h>
#include <entity/components/animation_component.h>
#include "graphics/opengl/framebuffer.h"

#include <glm/gtc/type_ptr.hpp>

#include "imgui_helper.h"

#include <iostream>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

namespace {
static std::string selectedDirectoryPath = "";


bool DeleteFilePath(const std::string& path) {
	try {
		// Check if the path exists
		if (!fs::exists(path)) {
			std::cerr << "The specified path does not exist: " << path << std::endl;
			return false;
		}
		
		// Remove the file or directory at the given path
		fs::remove_all(path); // use remove_all to also delete directories non-empty
		std::cout << "Successfully deleted: " << path << std::endl;
		return true;
	} catch (const fs::filesystem_error& e) {
		std::cerr << "Error deleting the file or directory: " << e.what() << std::endl;
		return false;
	}
}

// Helper function to generate a unique numeric-based filename
std::string GenerateUniqueFilename(const std::string& baseDir, const std::string& prefix, const std::string& extension) {
	int maxNumber = 0; // Track the highest number found
	std::regex filenamePattern("^" + prefix + "_(\\d+)\\." + extension + "$"); // Pattern to match filenames with numbers
	
	for (const auto& entry : fs::directory_iterator(baseDir)) {
		if (entry.is_regular_file()) {
			std::smatch match;
			std::string filename = entry.path().filename().string();
			
			if (std::regex_match(filename, match, filenamePattern) && match.size() == 2) {
				int number = std::stoi(match[1].str());
				maxNumber = std::max(maxNumber, number);
			}
		}
	}
	
	// Generate the new filename with the next number
	std::string newFilename = prefix + "_" + std::to_string(maxNumber + 1) + "." + extension;
	return baseDir + "/" + newFilename;
}

bool RenamePreview(const std::string& oldPath, const std::string& newName) {
	try {
		fs::path sourcePath(oldPath);
		fs::path targetPath = sourcePath.parent_path() / newName;
		if (fs::exists(targetPath)) {
			std::cerr << "Target file or directory already exists: " << targetPath << std::endl;
			return false;
		}
		return true;
	} catch (const fs::filesystem_error& e) {
		std::cerr << "Error renaming file or directory: " << e.what() << std::endl;
		return false;
	}
}

bool RenameFileOrDirectory(const std::string& oldPath, const std::string& newName) {
	try {
		fs::path sourcePath(oldPath);
		fs::path targetPath = sourcePath.parent_path() / newName;
		if (fs::exists(targetPath)) {
			std::cerr << "Target file or directory already exists: " << targetPath << std::endl;
			return false;
		}
		
		fs::rename(sourcePath, targetPath);
		std::cout << "Renamed " << oldPath << " to " << targetPath << std::endl;
		return true;
	} catch (const fs::filesystem_error& e) {
		std::cerr << "Error renaming file or directory: " << e.what() << std::endl;
		return false;
	}
}

const DirectoryNode* FindNodeByPath(const DirectoryNode& currentNode, const std::string& path) {
	if (currentNode.FullPath == path) {
		return &currentNode;
	}
	for (const auto& child : currentNode.Children) {
		if (const DirectoryNode* found = FindNodeByPath(child, path)) {
			return found;
		}
	}
	return nullptr; // Not found
}

void DisplayDirectoryNode(const DirectoryNode& node) {
	// If the node is a directory, use TreeNode to make it expandable/collapsible
	if (node.IsDirectory) {
		std::string folder;
		folder += ICON_MD_FOLDER;
		folder += " " + node.FileName;
		if (ImGui::TreeNode(folder.c_str(), ImGuiTreeNodeFlags_NoArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen | (selectedDirectoryPath == node.FullPath ? ImGuiTreeNodeFlags_Selected : 0))) {
			// Make the directory selectable
			if (ImGui::IsItemClicked()) {
				selectedDirectoryPath = node.FullPath; // Update the selected directory path
			}
			for (const auto& child : node.Children) {
				DisplayDirectoryNode(child);
			}
			ImGui::TreePop();
		}
	}
}


// Main function to draw the resources panel
bool DrawResourcesPanel(Scene *scene, ui::TimelineLayer& timelineLayer, ui::UiContext &ui_context, std::shared_ptr<anim::SharedResources> resources, const DirectoryNode& rootDirectoryNode, ui::DragSourceDetails& details) {
	
	if(ui_context.ai.is_clicked_new_scene){
		ui_context.scene.is_trigger_resources = true;
		selectedDirectoryPath = rootDirectoryNode.FullPath  + "/Scenes";
		
		std::string newSceneFilename = GenerateUniqueFilename(selectedDirectoryPath, "Scene", "pwr");
		
		// Create the new scene file
		std::ofstream newSceneFile(newSceneFilename);
		if (newSceneFile.is_open()) {
			newSceneFile.close();
			std::cout << "New scene file created: " << newSceneFilename << std::endl;
		} else {
			std::cerr << "Failed to create new scene file: " << newSceneFilename << std::endl;
		}
		
		resources->refresh_directory_node();
		
	} else if(ui_context.ai.is_clicked_new_composition){
		ui_context.scene.is_trigger_resources = true;
		selectedDirectoryPath = rootDirectoryNode.FullPath  + "/Compositions";
		
		// Generate a unique filename for the new sequence
		std::string newSequenceFilename = GenerateUniqueFilename(selectedDirectoryPath, "Sequence", "cmp");
		
		// Create the new sequence file
		std::ofstream newSequenceFile(newSequenceFilename);
		if (newSequenceFile.is_open()) {
			newSequenceFile.close();
			std::cout << "New sequence file created: " << newSequenceFilename << std::endl;
		} else {
			std::cerr << "Failed to create new sequence file: " << newSequenceFilename << std::endl;
		}
		
		resources->refresh_directory_node();
	} else if(ui_context.ai.is_clicked_new_sequence){
		ui_context.scene.is_trigger_resources = true;
		selectedDirectoryPath = rootDirectoryNode.FullPath  + "/Sequences";
		
		// Generate a unique filename for the new sequence
		std::string newSequenceFilename = GenerateUniqueFilename(selectedDirectoryPath, "Sequence", "seq");
		
		// Create the new sequence file
		std::ofstream newSequenceFile(newSequenceFilename);
		if (newSequenceFile.is_open()) {
			newSequenceFile.close();
			std::cout << "New sequence file created: " << newSequenceFilename << std::endl;
		} else {
			std::cerr << "Failed to create new sequence file: " << newSequenceFilename << std::endl;
		}
		
		resources->refresh_directory_node();
	}
	
	bool isHovered = false;
	
	ImGui::BeginChild("Resources");
	// Search box
	//	static char searchBuffer[256];
	//	ImGui::InputText("Search", searchBuffer, IM_ARRAYSIZE(searchBuffer));
	
	// Split view
	ImGui::Columns(2, "ResourcesColumns", false); // No border
	
	ImGui::SetColumnWidth(0, 852 / 4.0f);
	// Left column for directories
	ImGui::BeginChild("Directories", ImVec2(0, 0), true);
	
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
		isHovered = true;
	}
	
	ImFont* font = ImGui::GetFont();
	float original_scale = font->Scale; // Store the original scale
	
	// Increase the font scale
	font->Scale *= 1.25f; // Increase by 50%, adjust as needed
	
	ImGui::PushFont(font);
	
	DisplayDirectoryNode(rootDirectoryNode);
	
	font->Scale = original_scale;
	ImGui::PopFont();
	
	ImGui::EndChild();
	
	ImGui::NextColumn();
	
	// Right column for files (or selected directory's content)
	// For simplicity, this example won't dynamically change the right column based on selection.
	// Implementing this feature would require tracking the currently selected directory and displaying its contents.
	
	ImGui::BeginChild("Files", ImVec2(0, 0), true);
	
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
		isHovered = true;
	}
	
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	
	ImVec2 childFramePos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size = ImGui::GetContentRegionAvail();        // Resize canvas to what's available
	
	float totalItemHeight = 120;
	float totalItemWidth = 120;
	static std::string editingItemPath = "";
	static std::string previewItemPath = "";
	static std::string nameEditBuffer = "";
	static std::string deleteConfirmationFileName = "";
	char buffer[256];
	
	std::strncpy(buffer, nameEditBuffer.c_str(), sizeof(buffer));
	buffer[sizeof(buffer) - 1] = 0; // Ensure null termination
	
	auto fileHoverColor = IM_COL32(0, 0, 64, 64);
	static bool deleting = false;
	static bool isAnyFileHovered = false;
	
	isAnyFileHovered = false;
	
	if (!selectedDirectoryPath.empty()) {
		const DirectoryNode* selectedNode = FindNodeByPath(rootDirectoryNode, selectedDirectoryPath);
		if (selectedNode) {
			float spaceAvailable = ImGui::GetContentRegionAvail().x;
			float initialOffsetX = 30.0f; // Reasonable offset from the left
			float posX = initialOffsetX; // Current x position starts with the offset
			float posY = ImGui::GetCursorPosY(); // Start Y position
			
			// Spacing and sizing adjustments
			float itemInnerSpacing = 32.0f; // Increased inner spacing for more vertical separation
			float itemOuterSpacing = 64.0f;
			float verticalSeparation = 32.0f; // Additional space between the icon and the name
			
			for (const auto& child : selectedNode->Children) {
				std::string iconHeader = ICON_MD_PERSON;
				std::string iconSubHeader = "";

				if(child.IsDirectory){
					iconHeader = ICON_MD_FOLDER;
				}

				if(child.FileName.find(".fbx") != std::string::npos){
					iconHeader = ICON_MD_PERSON;
					
					if(selectedNode->FileName.find("Animations") != std::string::npos){
						iconHeader = ICON_MD_DIRECTIONS_WALK;
					}

				}
				if(child.FileName.find(".seq") != std::string::npos){
					iconHeader = ICON_MD_TIMER;
					
					if(selectedNode->FileName.find("Sequences") != std::string::npos){
						iconSubHeader = ICON_MD_PERSON;
					}
				}

				if(child.FileName.find(".cmp") != std::string::npos){
					iconHeader = ICON_MD_TIMER;
					
					if(selectedNode->FileName.find("Compositions") != std::string::npos){
						iconSubHeader += ICON_MD_PERSON;
						iconSubHeader += ICON_MD_PERSON;
						iconSubHeader += ICON_MD_PERSON;
					}
				}

				if(child.FileName.find(".pwr") != std::string::npos){
					iconHeader = ICON_MD_MOVIE_CREATION;
				}
				
				auto filePath = std::filesystem::path(child.FileName);
				
				filePath = filePath.replace_extension("");
				
				ImVec2 textSize = ImGui::CalcTextSize(filePath.string().c_str());
				ImVec2 iconSize = ImGui::CalcTextSize(iconHeader.c_str());
				ImVec2 headerSize = ImVec2(0, 0);
				
				if(!iconSubHeader.empty()){
					headerSize = ImGui::CalcTextSize(iconSubHeader.c_str());
				}
				
				// Wrap to next line if there's not enough space
				if (posX + totalItemWidth + itemOuterSpacing - initialOffsetX > spaceAvailable) {
					posY += totalItemHeight + itemOuterSpacing; // Move to next line
					posX = initialOffsetX; // Reset X position
				}
				
				ImVec2 p_min = ImVec2(posX + childFramePos.x - totalItemWidth / 4, posY + childFramePos.y);
				ImVec2 p_max = ImVec2(posX + totalItemWidth + childFramePos.x, posY + totalItemWidth + childFramePos.y);
				
				ImRect itemRect = {p_min, p_max};
				
				
				// Draw the frame around each item
				drawList->AddRect(p_min, p_max, IM_COL32(255, 255, 255, 255), 4.0f); // Added rounding for visual effect
				
				// Draw the icon centered within the frame
				float iconPosX = posX + (totalItemWidth - iconSize.x - 10) / 2;
				if(!iconSubHeader.empty()){
					iconPosX = posX + (totalItemWidth - headerSize.x - 10) / 2;
				}
				
				ImGui::SetCursorPos(ImVec2(posX, posY));
				
				const ImGuiIO& io = ImGui::GetIO();
				
				ImGui::Dummy(itemRect.GetSize());
				
				bool isFileHovered = itemRect.Contains(io.MousePos);
				
				if(!isAnyFileHovered){
					isAnyFileHovered = isFileHovered;
				}
				
				bool isEditingThisItem = (editingItemPath == child.FullPath);
				
				bool isEditableItem = (child.FileName.find(".seq") != std::string::npos || child.FileName.find(".cmp") != std::string::npos || child.FileName.find(".pwr") != std::string::npos);
				
				
				// Double-click to start editing
				if (isEditableItem && isFileHovered && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
					nameEditBuffer = filePath.filename().string();
					
					editingItemPath = child.FullPath;
					
				}
				
				if(isEditableItem && isEditingThisItem){
					auto ext = std::filesystem::path(child.FullPath).extension();
					
					std::string newName = buffer + ext.string(); // Simplified; add your renaming logic here
					if(RenamePreview(child.FullPath, previewItemPath)){
						fileHoverColor = IM_COL32(0, 0, 64, 64);
					} else {
						fileHoverColor = IM_COL32(64, 0, 0, 64);
					}
					
				}
				
				if(!isEditingThisItem && isEditableItem){
					fileHoverColor = IM_COL32(0, 0, 64, 64);
				}
				
				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)) && isEditableItem && isFileHovered && !deleting) {
					deleteConfirmationFileName = child.FullPath;
					deleting = true; // Enter deletion confirmation state
				} else if (deleting && deleteConfirmationFileName == child.FullPath && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete))) {
					// Confirm deletion on the second Delete key press for the same item
					DeleteFilePath(deleteConfirmationFileName); // Perform deletion
					resources->refresh_directory_node(); // Refresh the displayed nodes
					deleting = false; // Reset deletion state
					deleteConfirmationFileName.clear(); // Clear the path to prevent accidental reuse
				}
				
				if (deleting && deleteConfirmationFileName == child.FullPath) {
					fileHoverColor = IM_COL32(64, 0, 0, 64);
				} else {
					fileHoverColor = IM_COL32(0, 0, 64, 64);
				}
				
				
				if(isFileHovered && isHovered){
					drawList->AddRectFilled(p_min, p_max, fileHoverColor); // Added rounding for visual effect
					
					if(child.FileName.find(".seq") != std::string::npos){
						if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)){
							
							auto& cameras = scene->get_cameras();
							
							for(auto camera : cameras){
								camera->get_animation_tracks()->mTracks.clear();
							}
							
							auto sceneChldren = scene->get_mutable_shared_resources()->get_root_entity()->get_children_recursive();
							
							for(auto child : sceneChldren){
								child->get_animation_tracks()->mTracks.clear();
							}
							
							auto sequence = std::make_shared<anim::CinematicSequence>(*scene, resources, child.FullPath);
							
							auto& sequencer = sequence->get_sequencer();
							
							sequencer.mSequencerType = anim::TracksSequencer::SequencerType::Composition;
							
							timelineLayer.setCinematicSequence(sequence);
							
							ui_context.scene.editor_mode = ui::EditorMode::Composition;
							
							ui_context.scene.is_trigger_resources = false;
							ui_context.timeline.is_stop = true;
							ui_context.scene.is_mode_change = true;
						}
						if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)){
							details.isActive = true;
							details.type = "SEQUENCE_PATH";
							details.payload = child.FullPath;
							details.displayText = child.FileName;
						}
					} else if(child.FileName.find(".cmp") != std::string::npos){
						if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)){
							
							auto& cameras = scene->get_cameras();
							
							for(auto camera : cameras){
								camera->get_animation_tracks()->mTracks.clear();
							}
							
							auto sceneChldren = scene->get_mutable_shared_resources()->get_root_entity()->get_children_recursive();
							
							for(auto child : sceneChldren){
								child->get_animation_tracks()->mTracks.clear();
							}
							
							auto sequence = std::make_shared<anim::CinematicSequence>(*scene, resources, child.FullPath);
							
							auto& sequencer = sequence->get_sequencer();
							
							sequencer.mSequencerType = anim::TracksSequencer::SequencerType::Composition;
							
							timelineLayer.setCinematicSequence(sequence);
							
							ui_context.scene.editor_mode = ui::EditorMode::Composition;
							
							ui_context.scene.is_trigger_resources = false;
							ui_context.timeline.is_stop = true;
							ui_context.scene.is_mode_change = true;
						}
						if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)){
							details.isActive = true;
							details.type = "SEQUENCE_PATH";
							details.payload = child.FullPath;
							details.displayText = child.FileName;
						}
					}
					else if(child.FileName.find(".fbx") != std::string::npos){
						
						if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)){
							if(selectedNode->FileName.find("Animations") != std::string::npos){
								timelineLayer.clearActiveEntity();
								
								resources->import_model(child.FullPath.c_str());
								
								auto& animationSet = resources->getAnimationSet(child.FullPath);
								
								resources->add_animations(animationSet.animations);
								auto entity = resources->parse_model(animationSet.model, child.FileName.c_str());
								
								timelineLayer.setActiveEntity(ui_context, entity);
								
								auto sequence = std::make_shared<anim::AnimationSequence>(*scene, resources, entity, child.FullPath, animationSet.animations[0]->get_id());
								
								timelineLayer.setAnimationSequence(sequence);

								ui_context.scene.editor_mode = ui::EditorMode::Animation;
								
								ui_context.scene.is_trigger_resources = false;
								
								ui_context.entity.is_changed_selected_entity = true;
								ui_context.entity.selected_id = -1;
							}
							
						}
						
						if(selectedNode->FileName.find("Models") != std::string::npos){
							if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)){
								details.isActive = true;
								details.type = "FBX_ACTOR_PATH";
								details.payload = child.FullPath;
								details.displayText = child.FileName;
							}
						} else if(selectedNode->FileName.find("Animations") != std::string::npos){
							if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)){
								details.isActive = true;
								details.type = "FBX_ANIMATION_PATH";
								details.payload = child.FullPath;
								details.displayText = child.FileName;
							}
						}
					} else if(child.FileName.find(".pwr") != std::string::npos){
						
						if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)){
							timelineLayer.clearActiveEntity();
							ui_context.scene.editor_mode = ui::EditorMode::Map;
							ui_context.scene.is_trigger_resources = false;
							ui_context.timeline.is_stop = true;
							ui_context.scene.is_mode_change = false;
							
							scene->set_selected_entity(nullptr);
							
							resources->deserialize(child.FullPath);
							
							ui_context.scene.is_deserialize_scene = true;
							
							timelineLayer.deserialize_timeline();
						}
						
					}
					else if(child.IsDirectory){
						if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)){
							selectedDirectoryPath = child.FullPath;
						}
					}
				}
				
				
				
				ImGui::SetCursorPos(ImVec2(iconPosX, posY + itemInnerSpacing));
				
				if(!iconSubHeader.empty()){
					ImGui::SetCursorPosX(iconPosX + headerSize.x / 2.0f - iconSize.x / 2.0f);
				}
				
				ImGui::Text("%s", iconHeader.c_str());

				if(!iconSubHeader.empty()){
					ImGui::SetCursorPosX(iconPosX);
					ImGui::Text("%s", iconSubHeader.c_str());
				}
				
				float textPosX = posX + (totalItemWidth - textSize.x - 10) / 2;
				ImGui::SetCursorPos(ImVec2(textPosX, posY + iconSize.y + headerSize.y + itemInnerSpacing + verticalSeparation));
				
				// Callback function to set the caret position
				static auto SetCaretToEndCallback = [](ImGuiInputTextCallbackData* data) -> int {
					if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways) {
						data->CursorPos = data->BufTextLen;
						data->BufDirty = true;
					}
					return 0;
				};
				
				if (isEditableItem && isEditingThisItem) {
					
					auto textSize = ImGui::CalcTextSize(filePath.filename().string().c_str());
					
					float textWidth = textSize.x;
					float textHeight = textSize.y;
					
					float inputPosX = posX + (totalItemWidth - textWidth) / 2;
					ImGui::SetCursorPos(ImVec2(textPosX, posY + iconSize.y + itemInnerSpacing + verticalSeparation));
					
					if(isEditingThisItem){
						if (ImGui::InputTextEx("##edit", nullptr, buffer, sizeof(buffer), {textWidth, textHeight}, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackAlways, SetCaretToEndCallback)) {
							// Apply the new name here
							auto ext = std::filesystem::path(child.FullPath).extension();
							
							std::string newName = buffer + ext.string(); // Simplified; add your renaming logic here
							if (RenameFileOrDirectory(editingItemPath, newName)) {
								editingItemPath.clear(); // Successfully renamed, clear editing state
								resources->refresh_directory_node();
							}
						}
						
						auto ext = std::filesystem::path(child.FullPath).extension();
						
						std::string newName = buffer + ext.string(); // Simplified; add your renaming logic here
						
						previewItemPath = newName;
						ImGui::SetKeyboardFocusHere();
					}
					if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
						
						auto ext = std::filesystem::path(child.FullPath).extension();
						
						std::string newName = buffer + ext.string(); // Simplified; add your renaming logic here
						
						if (RenameFileOrDirectory(editingItemPath, newName)) {
							
							isEditingThisItem = false;
							editingItemPath = ""; // Exit edit mode
							
							resources->refresh_directory_node();
						}
						
					}
					
					
					// Optionally, handle Escape key to cancel editing
					if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
						editingItemPath.clear();
					}
					
					// Cancel editing if clicked away
					if (!ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
						editingItemPath = "";
					}
					
				} else {
					// Display clipped text if it's longer than 12 characters
					std::string displayName = filePath.filename().string();
					if (displayName.length() > 12) {
						displayName = displayName.substr(0, 9) + "..."; // Clip text with ellipsis
					}
					ImGui::SetCursorPos(ImVec2(textPosX, posY + iconSize.y + itemInnerSpacing + verticalSeparation));
					ImGui::Text("%s", displayName.c_str());
				}
				posX += totalItemWidth + itemOuterSpacing;
			}
			
			// Ensure cursor is moved down after the last item
			ImGui::Dummy(ImVec2(0.0, posY + totalItemHeight));
		}
		
		if(!isAnyFileHovered){
			if(!deleting){
				fileHoverColor = IM_COL32(0, 0, 64, 64);
				
				deleteConfirmationFileName.clear();
			} else {
				deleting = false;
			}
		}
	}
	
	ImGui::EndChild();
	
	ImGui::Columns(1);
	ImGui::EndChild();
	
	return isHovered;
}
}
using namespace anim;

namespace ui
{

void ResourcesLayer::draw(Scene *scene, UiContext &ui_context)
{
	init_context(ui_context, scene);
	
	if(selectedDirectoryPath.empty()){
		selectedDirectoryPath = scene->get_mutable_shared_resources()->getDirectoryNode().FullPath;
	}
	
	ImGuiWindowFlags window_flags = 0;
	
	window_flags |= ImGuiWindowFlags_NoTitleBar;
	window_flags |=
	ImGuiWindowFlags_NoMove;
	
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
	
	static float currentPos = viewportPanelSize.y;
	
	ImGui::SetNextWindowPos({0, currentPos});
	ImGui::SetNextWindowSize(scaled_mode_window_size);
	
	ImGui::Begin(ICON_MD_SCHEDULE " Resources", 0, window_flags | ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::GetCurrentWindow()->BeginOrderWithinContext = 100;
		
		AnimationComponent* anim_component = nullptr;
		
		// Define variables to control the animation
		static bool isScrollAnimationActive = false;
		static float scrollAnimationDuration = 1.0f; // Duration of the animation in milliseconds
		static float scrollAnimationStartTime = 0.0f;
		static float scrollAnimationStartPos = viewportPanelSize.y; // Initial scroll position
		static float scrollAnimationEndPos = viewportPanelSize.y - scaled_mode_window_size.y - 48.0f; // End scroll position (adjust as needed)
		
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(1.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 0.0f));
		
		// Inside your ImGui rendering loop or function
		if (ui_context.scene.is_trigger_resources) {
			if (!is_trigger_up_) {
				is_trigger_up_ = true;
				isScrollAnimationActive = true;
				
				scrollAnimationStartTime = 0;
				
				scrollAnimationDuration = scrollAnimationStartTime + 400;
				
				scrollAnimationStartPos = currentPos;
				
				scrollAnimationEndPos = viewportPanelSize.y - scaled_mode_window_size.y - 50.0f; // End scroll position (adjust as needed)
				
			}
			
			//ImGui::BeginChild(ICON_MD_DATA_OBJECT " Assets", {}, window_flags);
			
			if (isScrollAnimationActive) {
				
				
				scrollAnimationStartTime += ImGui::GetIO().DeltaTime * 1000.0f; // Convert seconds to milliseconds
				
				float progress = scrollAnimationStartTime / scrollAnimationDuration;
				
				// Ensure progress stays within [0, 1]
				progress = std::min(progress, 1.0f);
				
				
				// Interpolate the scroll position based on the progress
				currentPos = scrollAnimationStartPos + (scrollAnimationEndPos - scrollAnimationStartPos) * progress;
			}
			
		} else {
			if (is_trigger_up_) {
				is_trigger_up_ = false;
				isScrollAnimationActive = true;
				
				scrollAnimationStartTime = 0;
				
				scrollAnimationDuration = scrollAnimationStartTime + 400;
				
				scrollAnimationStartPos = currentPos;
				
				scrollAnimationEndPos = viewportPanelSize.y;
			}
			
			if (isScrollAnimationActive) {
				scrollAnimationStartTime += ImGui::GetIO().DeltaTime * 1000.0f; // Convert seconds to milliseconds
				
				float progress = scrollAnimationStartTime / scrollAnimationDuration;
				
				// Ensure progress stays within [0, 1]
				progress = std::min(progress, 1.0f);
				
				
				// Interpolate the scroll position based on the progress
				currentPos = scrollAnimationStartPos + (scrollAnimationEndPos - scrollAnimationStartPos) * progress;
			}
		}
		bool hovered = DrawResourcesPanel(scene, timeline_layer_, ui_context, scene->get_mutable_shared_resources(), scene->get_mutable_shared_resources()->getDirectoryNode(), dragSourceDetails_);
		
		if (dragSourceDetails_.isActive) {
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID | ImGuiDragDropFlags_SourceExtern)) {
				ImGui::SetDragDropPayload(dragSourceDetails_.type.c_str(), dragSourceDetails_.payload.c_str(), dragSourceDetails_.payload.size() + 1, ImGuiCond_Once);
				ImGui::Text("%s", dragSourceDetails_.displayText.c_str());
				ui_context.scene.is_trigger_resources = false;
				
				ImGui::EndDragDropSource();
			}
			
			// Reset drag source details after processing
			if(ImGui::IsMouseReleased(ImGuiMouseButton_Left)){
				dragSourceDetails_.isActive = false;
			}
		}
		
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !hovered && !ui_context.ai.is_widget_dragging) {
			if(ui_context.scene.is_trigger_resources == true){
				ui_context.scene.is_cancel_trigger_resources = true;
			}
		}
		//		// Render your content here
		//		RecursivelyDisplayDirectoryNode(timeline_layer_, ui_context, scene->get_mutable_shared_resources(), scene->get_mutable_shared_resources()->getDirectoryNode());
		
		//ImGui::EndChild();
		
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		
	}
	ImGui::End();
	
	
}
inline void ResourcesLayer::init_context(UiContext &ui_context, Scene *scene)
{
	auto &context = ui_context.timeline;
	scene_ = scene;
	resources_ = scene->get_mutable_shared_resources().get();
	
}

}
