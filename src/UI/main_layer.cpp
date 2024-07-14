#include "main_layer.h"
#include "ui_context.h"
#include "imgui_helper.h"
#include "scene_layer.h"

#include <entity/components/animation_component.h>
#include <entity/components/renderable/mesh_component.h>

#include <animation.h>

#include "text_edit_layer.h"

#include <array>
#include <glcpp/window.h>

#ifndef IMGUI_IMPL_OPENGL_LOADER_GLAD
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#endif

#include <imgui.h>
#include <ImGuizmo.h>
#include <icons/icons.h>
#include <fstream>
#include <sstream>

#include <ImGuiFileDialog/ImGuiFileDialog.h>
#ifdef _WIN32
#pragma warning(disable : 4005)
#include <Windows.h>
// #include <shellapi.h>
#include <lmcons.h>
// #pragma comment(lib, "Shell32.lib")
#else
#include <unistd.h>
#include <pwd.h>
#endif

#define FILTER_MODEL "Model files (*.fbx *.gltf){.fbx,.gltf,.glb}"

namespace {
// Base64 encoding table
static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];
	
	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;
			
			for(i = 0; (i <4) ; i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}
	
	if (i)
	{
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';
		
		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;
		
		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];
		
		while((i++ < 3))
			ret += '=';
		
	}
	
	return ret;
}

}

namespace ui
{
static bool isLinear{true};
static float ImportScale{100.0f};

MainLayer::MainLayer(Scene& scene, ImTextureID gearTexture, ImTextureID poweredByTexture, int width, int height)
: gearTextureId(gearTexture), poweredByTextureId(poweredByTexture), timeline_layer_(){
	
	gearPosition.x = width - 200;
	gearPosition.y = height - 200;
	
	dragStartPosition = gearPosition;
}

MainLayer::~MainLayer()
{
	shutdown();
}

void MainLayer::init()
{
	init_bookmark();
}
void MainLayer::init_bookmark()
{
	
	// define style for all directories
	ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeDir, "", ImVec4(0.02f, 0.02f, 0.02f, 1.0f), ICON_MD_FOLDER);
	// define style for all files
	ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByTypeFile, "", ImVec4(0.02f, .02f, 0.02f, 1.0f), ICON_IGFD_FILE);
	
#ifdef _WIN32
	wchar_t username[UNLEN + 1] = {0};
	DWORD username_len = UNLEN + 1;
	GetUserNameW(username, &username_len);
	std::wstring userPath = L"C:\\Users\\" + std::wstring(username) + L"\\";
	// Quick Access / Bookmarks
	ImGuiFileDialog::Instance()->AddBookmark(ICON_MD_MONITOR " Desktop", std::filesystem::path(userPath).append("Desktop").string());
	ImGuiFileDialog::Instance()->AddBookmark(ICON_MD_DESCRIPTION " Documents", std::filesystem::path(userPath).append("Documents").string());
	ImGuiFileDialog::Instance()->AddBookmark(ICON_MD_DOWNLOAD " Downloads", std::filesystem::path(userPath).append("Downloads").string());
	ImGuiFileDialog::Instance()->AddBookmark(ICON_MD_FAVORITE " OAC", std::filesystem::path("./").string());
#elif __APPLE__
	std::string user_name;
	user_name = "/Users/" + std::string(getenv("USER"));
	std::string homePath = user_name;
	if (std::filesystem::exists(homePath + "/Desktop"))
	{
		ImGuiFileDialog::Instance()->AddBookmark(ICON_MD_MONITOR " Desktop", std::filesystem::path(homePath + "/Desktop").string());
	}
	if (std::filesystem::exists(homePath + "/Documents"))
	{
		ImGuiFileDialog::Instance()->AddBookmark(ICON_MD_DESCRIPTION " Documents", homePath + "/Documents");
	}
	if (std::filesystem::exists(homePath + "/Downloads"))
	{
		ImGuiFileDialog::Instance()->AddBookmark(ICON_MD_DOWNLOAD " Downloads", homePath + "/Downloads");
	}
	ImGuiFileDialog::Instance()->AddBookmark(ICON_MD_FAVORITE " OAC", std::filesystem::path("./").string());
#endif
	std::ifstream wif("./bookmark");
	if (wif.good())
	{
		std::stringstream ss;
		ss << wif.rdbuf();
		ImGuiFileDialog::Instance()->DeserializeBookmarks(ss.str());
	}
	if (wif.is_open())
	{
		wif.close();
	}
}

void MainLayer::shutdown()
{
	std::ofstream bookmark_stream("./bookmark");
	std::string bookmark = ImGuiFileDialog::Instance()->SerializeBookmarks(false);
	bookmark_stream << bookmark;
	if (bookmark_stream.is_open())
	{
		bookmark_stream.close();
	}
}

void MainLayer::begin()
{
	ImGuizmo::SetOrthographic(false); // is perspective
	ImGuizmo::BeginFrame();
	
	bool persistedTriggerResources = context_.scene.is_trigger_resources;
	bool persistedWindowFocus = context_.scene.is_focused;
	bool persistedTriggerSetKeyframe = context_.scene.is_trigger_update_keyframe;
	bool persistedWasArmatureActivate = context_.scene.was_armature_activate;
	bool persistedWasMeshActivate = context_.scene.was_mesh_activate;
	bool persistedWasBonePickingMode = context_.scene.was_bone_picking_mode;
	ui::EditorMode persistedEditorMode = context_.scene.editor_mode;
	
	context_ = UiContext{};
	
	context_.scene.editor_mode = persistedEditorMode;
	context_.scene.is_trigger_resources = persistedTriggerResources;
	context_.scene.is_trigger_update_keyframe = persistedTriggerSetKeyframe;
	context_.scene.is_focused = persistedWindowFocus;

	context_.scene.was_armature_activate = persistedWasArmatureActivate;
	context_.scene.was_mesh_activate = persistedWasMeshActivate;
	context_.scene.was_bone_picking_mode = persistedWasBonePickingMode;

	if(context_.scene.editor_mode != ui::EditorMode::Animation){
		anim::ArmatureComponent::isActivate = false;
		anim::MeshComponent::isActivate = true;
		context_.scene.is_bone_picking_mode = false;
	} else {
		anim::MeshComponent::isActivate = context_.scene.was_mesh_activate;
		anim::ArmatureComponent::isActivate = context_.scene.was_armature_activate;
		context_.scene.is_bone_picking_mode = context_.scene.was_bone_picking_mode;
	}
	
}

void MainLayer::end()
{
}
bool showCircularMenu = false;
ImVec2 menuCenter;
float menuRadius = 100.0f; // Example radius for the circular menu
static bool isChatboxVisible = false; // Flag to track chatbox visibility

void MainLayer::draw_ai_widget(Scene* scene)
{
	ImVec4* colors = ImGui::GetStyle().Colors;
	ImVec4 oldButton = colors[ImGuiCol_Button];
	ImVec4 oldButtonHovered = colors[ImGuiCol_ButtonHovered];
	ImVec4 oldButtonActive = colors[ImGuiCol_ButtonActive];
	colors[ImGuiCol_Button] = ImVec4(0, 0, 0, 0); // Transparent
	colors[ImGuiCol_ButtonHovered] = ImVec4(0, 0, 0, 0); // Transparent
	colors[ImGuiCol_ButtonActive] = ImVec4(0, 0, 0, 0); // Transparent
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0)); // No padding
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
	// After drawing your UI:
	
	// Calculate the window size based on the menu radius to ensure everything fits
	float windowSize = menuRadius * 2.0f + gearSize.x; // Ensure the window is large enough for the menu
	ImVec2 windowPosition = ImVec2(gearPosition.x - windowSize / 2.0f, gearPosition.y - windowSize / 2.0f);
	
	ImGui::SetNextWindowPos(windowPosition);
	ImGui::SetNextWindowSize({windowSize, windowSize});
	
	auto windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground;
	
	static bool isWidgetActive = false;
	
	static bool showComponentSubMenu = false;
	static bool showAssetSubMenu = false;
	

	ImGui::Begin("Texture Button", nullptr, windowFlags);
	
	ImGui::GetCurrentWindow()->BeginOrderWithinContext = 4096;
	
	if (gearTextureId) {
		
		const int numItems = 4; // Updated number of items in the circular menu
		ImVec2 itemSize = ImVec2(90, 30); // Updated item size for better visualization
		const char* itemLabels[] = {"Import", "Prompt", "Asset", "Component"}; // Updated item labels
		ImVec4 itemColors[] = {
			ImVec4(0.0f, 0.61f, 0.93f, 1.0f),   // Sky Blue for "Import"
			ImVec4(0.90f, 0.49f, 0.13f, 1.0f), // Orange
			ImVec4(0.82f, 0.17f, 0.31f, 1.0f), // Red
			ImVec4(0.58f, 0.20f, 0.82f, 1.0f)  // New Color for "Import"
		};
		ImVec2 buttonPos = ImVec2((windowSize - gearSize.x) / 2.0f, (windowSize - gearSize.y) / 2.0f);
		
		animationProgress += showCircularMenu ? animationSpeed : -animationSpeed;
		animationProgress = std::clamp(animationProgress, 0.0f, 1.0f);

		// Calculate current radius based on animation progress
		// This adjusts whether we are showing or hiding the menu
		float currentMenuRadius = menuRadius * animationProgress;
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);

		if (animationProgress > 0.0f) { // Render the menu if there's any animation progress
			
			for (int i = 0; i < numItems; ++i) {
				// Angle calculation and item rendering remains the same...
				float angles[] = {0, M_PI / 2, M_PI, 3 * M_PI / 2};
				float angle = angles[i];
				
				ImVec2 itemCenter = ImVec2(
										   menuCenter.x + cos(angle) * currentMenuRadius - itemSize.x / 2.0f,
										   menuCenter.y + sin(angle) * currentMenuRadius - itemSize.y / 2.0f
										   );
				
				ImGui::SetCursorScreenPos(itemCenter);
				
				ImVec4 baseColor = itemColors[i];
				ImVec4 hoverColor = baseColor; // Start with the base color
				
				// Lighten
				float lightenFactor = 0.2f; // Increase RGB by 20%
				hoverColor.x = hoverColor.x + lightenFactor * (1.0f - hoverColor.x);
				hoverColor.y = hoverColor.y + lightenFactor * (1.0f - hoverColor.y);
				hoverColor.z = hoverColor.z + lightenFactor * (1.0f - hoverColor.z);
				
				ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
				
				if (ImGui::Button(itemLabels[i], itemSize)) {
					if(i == 3){ 
						showComponentSubMenu = !showComponentSubMenu;
						showAssetSubMenu = false; // Optionally close other sub-menus

						// clicked scene
						//context_.ai.is_clicked_new_scene = true;
					} else if(i == 2){ // clicked scene
						
						showAssetSubMenu = !showAssetSubMenu;
						showComponentSubMenu = false; // Optionally close other sub-menus

					} else if(i == 1){
						context_.ai.is_clicked_ai_prompt = true;
						isChatboxVisible = true;
					}
					showCircularMenu = false;
				}
				
				ImGui::PopStyleColor(2);
			}
			
		}
		
		// If the main menu is fully collapsed, start showing sub-menu items
		if (animationProgress == 0.0f && showComponentSubMenu) {
			
			const int numSubItems = 3;
			const char* subItemLabels[] = {ICON_MD_LIGHTBULB, ICON_MD_VIDEO_CAMERA_FRONT, ICON_MD_PERSON};
			ImVec4 itemColors[] = {
				ImVec4(0.8f, 0.8f, 0.0f, 1.0f), // Yellow
				ImVec4(0.3f, 0.3f, 0.3f, 1.0f), // Gray
				ImVec4(0.0f, 0.5f, 0.0f, 1.0f)  // Green
			};


			for (int j = 0; j < numSubItems; ++j) {
				
				ImVec4 baseColor = itemColors[j];
				ImVec4 hoverColor = baseColor; // Start with the base color
				
				float lightenFactor = 0.2f; // Increase RGB by 20%
				hoverColor.x = hoverColor.x + lightenFactor * (1.0f - hoverColor.x);
				hoverColor.y = hoverColor.y + lightenFactor * (1.0f - hoverColor.y);
				hoverColor.z = hoverColor.z + lightenFactor * (1.0f - hoverColor.z);
				
				ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
				
				// Angle calculation and item rendering remains the same...
				float angles[] = {0.5 * (M_PI / 3), 2.5 * (M_PI / 3), 3 * M_PI / 2};
				float offsetsY[] = {24, 24, 0};
				float angle = angles[j];
				
				ImVec2 itemCenter = ImVec2(
										   menuCenter.x + cos(angle) * menuRadius - itemSize.x / 2.0f,
										   menuCenter.y + sin(angle) * menuRadius - itemSize.y / 2.0f + offsetsY[j]
										   );
				
				ImGui::SetCursorScreenPos(itemCenter);
				
				if (ImGui::Button(subItemLabels[j], ImVec2(90, 30))) {
					if(j == 0){
						context_.ai.is_clicked_new_light = true;
					} else if(j == 1){
						context_.ai.is_clicked_new_camera = true;
					} else if(j == 2){
						context_.ai.is_clicked_add_physics = true;
					}
					
					showComponentSubMenu = false;
					showCircularMenu = false;

				}
				
				ImGui::PopStyleColor(2);

			}
		}
		
		if (animationProgress == 0.0f && showAssetSubMenu) {
			// Correcting the calculation for componentMenuCenter
			ImVec2 componentMenuCenter = ImVec2(
												menuCenter.x + cos(3 * M_PI_2) * menuRadius, // Corrected
												menuCenter.y + sin(3 * M_PI_2) * menuRadius  // Corrected
												);
			
			const int numSubItems = 3;
			const char* subItemLabels[] = {"Sequence", "Composition", "Scene"};
			ImVec4 itemColors[] = {
				ImVec4(0.8f, 0.0f, 0.8f, 1.0f),
				ImVec4(0.0f, 0.3f, 0.3f, 1.0f), // Gray
				ImVec4(0.0f, 0.0f, 0.3f, 1.0f)  // Green
			};
			
			
			for (int j = 0; j < numSubItems; ++j) {
				
				ImVec4 baseColor = itemColors[j];
				ImVec4 hoverColor = baseColor; // Start with the base color
				
				float lightenFactor = 0.2f; // Increase RGB by 20%
				hoverColor.x = hoverColor.x + lightenFactor * (1.0f - hoverColor.x);
				hoverColor.y = hoverColor.y + lightenFactor * (1.0f - hoverColor.y);
				hoverColor.z = hoverColor.z + lightenFactor * (1.0f - hoverColor.z);
				
				ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
				
				// Angle calculation and item rendering remains the same...
				float angles[] = {0.5 * (M_PI / 3), 2.5 * (M_PI / 3), 3 * M_PI / 2};
				float offsetsY[] = {24, 24, 0};
				float angle = angles[j];
				
				ImVec2 itemCenter = ImVec2(
										   menuCenter.x + cos(angle) * menuRadius - itemSize.x / 2.0f,
										   menuCenter.y + sin(angle) * menuRadius - itemSize.y / 2.0f + offsetsY[j]
										   );
				
				ImGui::SetCursorScreenPos(itemCenter);
				
				if (ImGui::Button(subItemLabels[j], ImVec2(90, 30))) {
					if(j == 0){
						context_.ai.is_clicked_new_sequence = true;
					} else if(j == 1){
						context_.ai.is_clicked_new_composition = true;
					} else if(j == 2){
						context_.ai.is_clicked_new_scene = true;
					}

					showAssetSubMenu = false;
					showCircularMenu = false;
				}
				
				ImGui::PopStyleColor(2);
				
			}
		}


		ImGui::PopStyleVar();

		
		ImVec4 tintCol = ImVec4(1, 1, 1, 1); // Normal tint color (white, no tint)
		ImVec4 hoverTintCol = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // Light grey tint for hover
		
		// Adjust tint color based on hover state before drawing the button
		
		ImGui::SetCursorPos(buttonPos);
		
		auto hoverPos = ImGui::GetCursorScreenPos();
		
		ImRect hoveringRect =
		{
			{
				hoverPos.x + 8,
				hoverPos.y + 8
			},
			{
				hoverPos.x + gearSize.x - 8,
				hoverPos.y + gearSize.y - 8
			}
		};
		
		if (hoveringRect.Contains(ImGui::GetIO().MousePos)) {
			hoverTintCol = tintCol;
		}
		
		bool draggingAndReleasing = ImGui::IsMouseReleased(ImGuiMouseButton_Left) && draggingThisFrame;
		
		if(ImGui::IsMouseReleased(ImGuiMouseButton_Left)){
			if(isWidgetActive){
				isWidgetActive = false;
			}
		}
		
		if (ImGui::ImageButton(gearTextureId, gearSize, ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), hoverTintCol)) {
			// Check if gear position hasn't changed significantly and it's not a drag release action
			if (dragStartPosition.x == gearPosition.x && dragStartPosition.y == gearPosition.y && !draggingAndReleasing) {
				if (showComponentSubMenu) {
					showComponentSubMenu = false;
					showCircularMenu = true;
				} else if(showAssetSubMenu){
					showAssetSubMenu = false;
					showCircularMenu = true;
				} else {
					// If no sub-menus are open, toggle the main menu visibility
					showCircularMenu = !showCircularMenu;
				}

			}
		}
		
		if (hoveringRect.Contains(ImGui::GetIO().MousePos)) {
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				// When mouse is clicked, record the start position
				dragStartPosition = gearPosition;
				
				isWidgetActive = true;
			}
		}
		
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && isWidgetActive) {
			ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
			gearPosition.x += dragDelta.x;
			gearPosition.y += dragDelta.y;
			draggingThisFrame = true;
			ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left); // Reset drag delta after applying the drag
		} else {
			draggingThisFrame = false;
		}
		
		if(draggingThisFrame){
			showCircularMenu = false;
		}
		
		if(showCircularMenu){
			gearSize = ImVec2(120, 120); // Slightly larger size when clicked
			menuRadius = gearSize.x;
			
		} else {
			gearSize = ImVec2(100, 100);
			menuRadius = gearSize.x;
		}
		
		menuCenter.x = gearPosition.x;
		menuCenter.y = gearPosition.y;
	} else {
		ImGui::Text("Failed to load texture.");
	}
	
	ImGui::End();
	ImGui::PopStyleVar(2);
	ImGui::PopStyleVar(); // Pop FramePadding
	colors[ImGuiCol_Button] = oldButton;
	colors[ImGuiCol_ButtonHovered] = oldButtonHovered;
	colors[ImGuiCol_ButtonActive] = oldButtonActive;
	
	
	context_.ai.is_widget_dragging = isWidgetActive || draggingThisFrame;
	if (isChatboxVisible) {
		// Ensure the chatbox window is positioned and sized appropriately
		ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_FirstUseEver); // Example size, adjust as needed
		ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver); // Example position, adjust as needed
		ImGui::Begin("AI Prompt", &isChatboxVisible);
		ImGui::GetCurrentWindow()->BeginOrderWithinContext = 4096;
		
		// Chat and Render Target layout in two columns
		ImGui::Columns(2, NULL, true);
		
		// Left column - Chatbox
		ImGui::Text("Prompt History");
		static char message[128] = "";
		ImGui::InputText("##Message", message, IM_ARRAYSIZE(message));
		ImGui::SameLine();
		
		
		// Chat history (example of a vertical chatbox)
		static std::vector<std::string> chatHistory;

		if (ImGui::Button("Send")) {
			auto res = _client.Get("/account/v1/creditBalance");
			
			_sessionCookie = res->body;
			
			Json::CharReaderBuilder readerBuilder;
			Json::CharReader* reader = readerBuilder.newCharReader();
			Json::Value jsonData;
			std::string errors;

			
			bool parsingSuccessful = reader->parse(res->body.c_str(), res->body.c_str() + res->body.size(), &jsonData, &errors);

			// Check for parsing errors
			if (!parsingSuccessful) {
				chatHistory.push_back(std::string("Failed to parse the JSON string: "));
			} else {
//				
//				{"credits":<value>,"subscription":{"name":<value>,"credits":<value>,"featureLimits":{"maxVariantsGeneration":<value>},"currentPeriod":{"start":<value>,"end":<value>}}}
//				

				chatHistory.push_back(std::string(jsonData["credits"].asString()));
			}

			// Add message to chat history
			// Clear the input
			message[0] = '\0';
		}

		for (const auto& msg : chatHistory) {
			ImGui::TextUnformatted(msg.c_str());
		}
				
		ImGui::NextColumn();
		
		float columnWidth = ImGui::GetColumnWidth();
		float imageSize = columnWidth < ImGui::GetWindowHeight() ? columnWidth : ImGui::GetWindowHeight();

		// Right column - Render target logic (e.g., 3D model) and Settings button
		if (scene) {
			ImGui::Text("Preview");

			ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(scene->get_mutable_framebuffer()->get_color_attachment())),
						 ImVec2{imageSize, imageSize},
						 ImVec2{0, 1},
						 ImVec2{1, 0});

			// Drag and Drop logic
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				ImGui::SetDragDropPayload("RENDER_TARGET", scene, sizeof(scene)); // You might need to adjust the payload depending on what you need to transfer
				ImGui::Text("Dragging Render Target");
				ImGui::EndDragDropSource();
			}
			
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RENDER_TARGET")) {
					// Handle the dropped payload
					// auto dropped_scene = *static_cast<decltype(scene)*>(payload->Data);
					// Do something with the dropped scene
				}
				ImGui::EndDragDropTarget();
			}
		}
		
		if (ImGui::Button("Settings")) {
			ImGui::OpenPopup("AI Settings");
		}
		
		// Settings modal
		if (ImGui::BeginPopupModal("AI Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::GetCurrentWindow()->BeginOrderWithinContext = 8196;

			// Center the image
			float windowWidth = ImGui::GetWindowSize().x;
			float imageWidth = 128; // Adjust the image width as needed
			float imageHeight = 128; // Adjust the image height as needed
			float imageX = (windowWidth - imageWidth) * 0.5f;
			
			ImGui::SetCursorPosX(imageX);
			ImGui::Image(poweredByTextureId, ImVec2(imageWidth, imageHeight));
			
			// Align input fields
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			
			static char clientId[128] = "";
			static char clientSecret[128] = "";
			
			ImGui::InputText("##ClientID", clientId, IM_ARRAYSIZE(clientId));
			ImGui::Text("Client ID");
			
			ImGui::Spacing();
			ImGui::InputText("##ClientSecret", clientSecret, IM_ARRAYSIZE(clientSecret));
			ImGui::Text("Client Secret");
			
			
			static std::string authMessage = "";
			static ImVec4 authColor;

			
			ImGui::Spacing();
			ImGui::Spacing();
			
			
			// Test Credentials button
			if (ImGui::Button("Test")) {
				// Encode clientId and clientSecret in Base64
				std::string credentials = std::string(clientId) + ":" + std::string(clientSecret);
				std::string encodedCredentials = base64_encode(reinterpret_cast<const unsigned char*>(credentials.c_str()), credentials.length());
				
				_client.set_default_headers({
					{ "Authorization", "Basic " + encodedCredentials }
				});
				
				auto res = _client.Get("/account/v1/auth");
				
				
				if (res && res->status == httplib::StatusCode::OK_200) {
					_sessionCookie = res->headers.find("Set-Cookie")->second;
					
					_client.set_default_headers({
						{ "cookie", _sessionCookie }
					});
					
					// Handle successful authentication
					authMessage = "Credentials are valid!";
					
					authColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green color for success
					
				} else {
					// Handle error
					authMessage = "Invalid credentials.";
					
					authColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red color for error
					
				}
			}
			
			ImGui::SameLine();

			if (ImGui::Button("Close")) {
				ImGui::CloseCurrentPopup();
				
				// Encode clientId and clientSecret in Base64
				std::string credentials = std::string(clientId) + ":" + std::string(clientSecret);
				std::string encodedCredentials = base64_encode(reinterpret_cast<const unsigned char*>(credentials.c_str()), credentials.length());
				
				_client.set_default_headers({
					{ "Authorization", "Basic " + encodedCredentials }
				});
				
				auto res = _client.Get("/account/v1/auth");
				
				if (res && res->status == httplib::StatusCode::OK_200) {
					_sessionCookie = res->headers.find("Set-Cookie")->second;
					
					_client.set_default_headers({
						{ "cookie", _sessionCookie }
					});
				} else {
					// Handle error
				}
			}
			if (!authMessage.empty()) {
				ImGui::SameLine();
				ImGui::TextColored(authColor, "%s", authMessage.c_str());
			}

			ImGui::EndPopup();

		}
		
		ImGui::End(); // End of chatbox window
	}
}

void MainLayer::draw_ingame_menu(Scene* scene){
	ImGuiIO &io = ImGui::GetIO();
	
	// Define the original mode_window_size
	
	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
	
	ImVec2 btn_size{viewportPanelSize.x / 4, 45.0f};
	
	ImVec2 mode_window_size = {viewportPanelSize.x, btn_size.y};
	ImVec2 mode_window_pos = {0, viewportPanelSize.y - btn_size.y};
	
	
	// Calculate scaling factors for both x and y dimensions
	float scaleX = viewportPanelSize.x / btn_size.x;
	float scaleY = viewportPanelSize.y / btn_size.y;
	
	// Choose the smaller scaling factor to maintain aspect ratio
	float scale = std::min(scaleX, scaleY);
	
	// Scale the mode_window_size
	ImVec2 scaled_mode_window_size = {
		btn_size.x * scale,
		btn_size.y * scale
	};
	
	ImGui::SetNextWindowSize(scaled_mode_window_size);
	ImGui::SetNextWindowPos(mode_window_pos);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, mode_window_size);
	ImGui::PushStyleColor(ImGuiCol_Button, {0.3f, 0.3f, 0.3f, 0.8f});
	ImGui::PushStyleColor(ImGuiCol_Text, {1.0f, 1.0f, 1.0f, 1.0f});
	if (ImGui::Begin("Child", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
	{
		ImGui::GetCurrentWindow()->BeginOrderWithinContext = 105;
		
		if (ImGui::BeginTable("split", 1, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_SizingFixedSame))
		{
			ImGui::TableNextColumn();
			
			bool isCancelTriggerResources = context_.scene.is_cancel_trigger_resources;
			bool isTriggerResources = context_.scene.is_trigger_resources;
			bool isCompositionMode = context_.scene.editor_mode == ui::EditorMode::Composition;
			bool isAnimationMode = context_.scene.editor_mode == ui::EditorMode::Animation;

			bool isBluePrintMode = context_.scene.editor_mode == ui::EditorMode::Map;

			bool setBluePrintMode = false;
			bool setTriggerResources = false;
			bool setCompositionMode = false;
			bool setAnimationMode = false;
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.f, 0.f});
			
			ImGui::PushFont(io.Fonts->Fonts[ICON_FA]);
			ToggleButton(ICON_FA_OBJECT_GROUP, &isTriggerResources, {btn_size.x, btn_size.y}, &setTriggerResources);
			
			ImGui::SameLine();
			
			ImGui::PopFont();
			
			ImGui::PushFont(io.Fonts->Fonts[ICON_FA]);
			ToggleButton(ICON_FA_MAP, &isBluePrintMode, {btn_size.x, btn_size.y}, &setBluePrintMode);
			
			ImGui::SameLine();
			
			ImGui::PopFont();

			
			ToggleButton(ICON_MD_TRACK_CHANGES, &isCompositionMode, btn_size, &setCompositionMode);
			
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SEQUENCE_PATH")) {
					const char* sequencePath = reinterpret_cast<const char*>(payload->Data);
					
					auto sequence = std::make_shared<anim::CinematicSequence>(*scene, scene->get_mutable_shared_resources(), sequencePath);
					
					timeline_layer_.setCinematicSequence(sequence);
					
					context_.scene.editor_mode = ui::EditorMode::Composition;
					
					context_.scene.is_trigger_resources = false;
					context_.timeline.is_stop = true;
					context_.scene.is_mode_change = true;
				}
				
				ImGui::EndDragDropTarget();
			}
			
			ImGui::SameLine();
			
			ImGui::PushFont(io.Fonts->Fonts[ICON_FA]);
			
			ToggleButton(ICON_FA_CLOCK, &isAnimationMode, btn_size, &setAnimationMode);
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBX_ANIMATION_PATH")) {
					
					const char* fbxPath = reinterpret_cast<const char*>(payload->Data);
					
					auto resources = scene->get_mutable_shared_resources();
					
					timeline_layer_.clearActiveEntity();
					
					resources->import_model(fbxPath);

					auto& animationSet = resources->getAnimationSet(fbxPath);
					
					resources->add_animations(animationSet.animations);
					
					auto entity = resources->parse_model(animationSet.model, fbxPath);
										
					timeline_layer_.setActiveEntity(context_, entity);
					
					auto sequence = std::make_shared<anim::AnimationSequence>(*scene, scene->get_mutable_shared_resources(), entity, fbxPath, animationSet.animations[0]->get_id());

					timeline_layer_.setAnimationSequence(sequence);
					
					context_.scene.editor_mode = ui::EditorMode::Animation;
					
					context_.scene.is_trigger_resources = false;
				}
				
				ImGui::EndDragDropTarget();
			}
			
			if(isTriggerResources){
				context_.scene.is_trigger_resources = true;
			} else {
				context_.scene.is_trigger_resources = false;
			}
			
			if(isCancelTriggerResources){
				context_.scene.is_trigger_resources = false;
			}
			
			if(setBluePrintMode){
				isAnimationMode = false;
				isCompositionMode = false;
				context_.scene.editor_mode = EditorMode::Map;
				context_.timeline.is_stop = true;
				context_.scene.is_mode_change = true;
			}
			
			if(setCompositionMode){
				isAnimationMode = false;
				isBluePrintMode = false;
				context_.scene.editor_mode = EditorMode::Composition;
				
				context_.timeline.is_stop = true;
				context_.scene.is_mode_change = true;
			}
			
			if(setAnimationMode){
				isCompositionMode = false;
				isBluePrintMode = false;

				context_.scene.editor_mode = EditorMode::Animation;
				
				context_.timeline.is_stop = true;
				context_.scene.is_mode_change = true;
			}
			
			if(context_.scene.is_mode_change){
				
				if(context_.scene.editor_mode == EditorMode::Composition || context_.scene.editor_mode == EditorMode::Map){
					scene->set_selected_entity(0);
				} else {
					context_.entity.is_changed_selected_entity = true;
					context_.entity.selected_id = -1;
					
					scene->set_selected_entity(nullptr);
				}
			}
			
			ImGui::PopStyleVar();
			ImGui::PopFont();
			
			ImGui::EndTable();
		}
	}
	ImGui::End();
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar();
}

inline void LinearInfosPane(const char *vFilter, IGFDUserDatas vUserDatas, bool *vCantContinue) // if vCantContinue is false, the user cant validate the dialog
{
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Export");
	ImGui::Text("Linear: ");
	ImGui::SameLine();
	ImGui::Checkbox("##check", reinterpret_cast<bool *>(vUserDatas));
}

void MainLayer::draw_menu_bar(float fps)
{
	const char *menu_dialog_name[5] = {
		"Import Model",
		"Import Animation",
		"ImportDir",
		"Export",
		"ExportData"};
	std::array<bool *, 5> is_clicked_dir = {
		&context_.menu.is_clicked_import_model,
		&context_.menu.is_clicked_import_animation,
		&context_.menu.is_clicked_import_dir,
		&context_.menu.is_clicked_export_animation,
		&context_.menu.is_clicked_export_all_data};
	ImVec2 minSize = {650.0f, 400.0f}; // Half the display area
	const char *filters = FILTER_MODEL ",Json Animation (*.json){.json},.*";
	static bool py_modal = false;
	
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Import: model", NULL, nullptr))
			{
				ImGuiFileDialog::Instance()->OpenDialog(menu_dialog_name[0],
														ICON_MD_FILE_OPEN " Open fbx, gltf ...",
														filters,
														".", 1, nullptr,
														// std::bind(&ScaleInfosPane, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 150, 1,
														// IGFD::UserDatas(&ImportScale),
														ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_DisableCreateDirectoryButton);
			}
			if (ImGui::MenuItem("Import: animation", NULL, nullptr))
			{
				ImGuiFileDialog::Instance()->OpenDialog(menu_dialog_name[1],
														ICON_MD_FILE_OPEN " Open fbx, gltf ...",
														filters,
														".", 1, nullptr,
														// std::bind(&ScaleInfosPane, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 150, 1,
														// IGFD::UserDatas(&ImportScale),
														ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_DisableCreateDirectoryButton);
			}
			if (ImGui::MenuItem("Import: Folder", NULL, nullptr))
			{
				ImGuiFileDialog::Instance()->OpenDialog(menu_dialog_name[2], "Choose a Directory",
														nullptr,
														".", 1, nullptr,
														// std::bind(&ScaleInfosPane, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 150, 1,
														// IGFD::UserDatas(&ImportScale),
														ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_DisableCreateDirectoryButton);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Export: animation(selected model)", NULL, nullptr))
			{
				ImGuiFileDialog::Instance()->OpenDialog(menu_dialog_name[3], "Save",
														FILTER_MODEL,
														".", "",
														std::bind(&LinearInfosPane, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 150, 1,
														IGFD::UserDatas(&isLinear),
														ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ConfirmOverwrite);
			}
			// for data extract (for deep learning)
			// if (ImGui::MenuItem("Export: rotation, world pos(json)", NULL, nullptr))
			// {
			//     ImGuiFileDialog::Instance()->OpenDialog(menu_dialog_name[3], "Save",
			//                                             "Json (*.json){.json}",
			//                                             ".", 1, nullptr,
			//                                             ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ConfirmOverwrite);
			// }
			ImGui::EndMenu();
		}
		//            if (ImGui::BeginMenu("Python"))
		//            {
		//                if (ImGui::MenuItem("Mediapipe", NULL, nullptr))
		//                {
		//                    py_modal = true;
		//                }
		//
		//                ImGui::EndMenu();
		//            }
		
		//            ImGui::Text("fps: %.2f", fps);
		
		ImGui::EndMenuBar();
	}
	//        draw_python_modal(py_modal);
	int dialog_count = 0;
	for (int i = 0; i < 4; i++)
	{
		if (ImGuiFileDialog::Instance()->Display(menu_dialog_name[i], ImGuiWindowFlags_NoCollapse, minSize))
		{
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				context_.menu.path = ImGuiFileDialog::Instance()->GetFilePathName();
				*is_clicked_dir[i] = true;
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}
	if (ImGuiFileDialog::Instance()->IsOpened())
	{
		context_.menu.is_dialog_open = true;
	}
	context_.menu.is_export_linear_interpolation = isLinear;
	context_.menu.import_scale = ImportScale;
}
//    void MainLayer::draw_python_modal(bool &is_open)
//    {
//        static std::string video_path = "";
//        static std::string save_path = std::filesystem::absolute("./animation.json").string();
//        video_path.resize(200);
//        save_path.resize(200);
//        if (is_open)
//        {
//            ImGui::OpenPopup("Mediapipe");
//            is_open = false;
//        }
//        ImGuiStyle &style = ImGui::GetStyle();
//        auto color = style.Colors[ImGuiCol_Button];
//        color.x = 1.0f - color.x;
//        color.y = 1.0f - color.y;
//        color.z = 1.0f - color.z;
//        ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
//        if (ImGui::BeginPopupModal("Mediapipe", NULL, ImGuiWindowFlags_AlwaysAutoResize))
//        {
//            context_.menu.is_dialog_open = true;
//            ImGui::Text("You must have a selected model.");
//            ImGui::Text("Video:");
//            ImGui::SameLine();
//            auto text_cursor = ImGui::GetCursorPosX();
//            char *path = video_path.data();
//            ImGui::InputText("##video_path", path, video_path.size());
//            ImGui::SameLine();
//            auto current_cursor = ImGui::GetCursorPosX();
//            if (ImGui::Button("Open"))
//            {
//                ImGuiFileDialog::Instance()->OpenDialog("ChooseVideo", "Choose a Video",
//                                                        "Video (*.mp4 *.gif){.mp4,.gif,.avi},.*",
//                                                        ".", 1, nullptr,
//                                                        ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_DisableCreateDirectoryButton);
//            }
//            if (ImGuiFileDialog::Instance()->Display("ChooseVideo", ImGuiWindowFlags_NoCollapse, {650.0f, 400.0f}))
//            {
//                if (ImGuiFileDialog::Instance()->IsOk())
//                {
//                    video_path = ImGuiFileDialog::Instance()->GetFilePathName();
//                }
//                ImGuiFileDialog::Instance()->Close();
//            }
//
//            ImGui::Text("Save:");
//            ImGui::SameLine(text_cursor);
//            char *s_path = save_path.data();
//            ImGui::InputText("##save_path", s_path, save_path.size());
//            ImGui::SameLine(current_cursor);
//            if (ImGui::Button("Open##2"))
//            {
//                ImGuiFileDialog::Instance()->OpenDialog("ChooseJson", "Choose a Json",
//                                                        "JSON (*.json){.json},.*",
//                                                        ".", 1, nullptr,
//                                                        ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_DisableCreateDirectoryButton);
//            }
//            if (ImGuiFileDialog::Instance()->Display("ChooseJson", ImGuiWindowFlags_NoCollapse, {650.0f, 400.0f}))
//            {
//                if (ImGuiFileDialog::Instance()->IsOk())
//                {
//                    save_path = ImGuiFileDialog::Instance()->GetFilePathName();
//                }
//                ImGuiFileDialog::Instance()->Close();
//            }
//            ImGui::Separator();
//            ImGui::Text("Factor");
//            ImGui::NewLine();
//            ImGui::SameLine(text_cursor);
//            ImGui::DragFloat("##factor", &context_.python.factor, 0.1f, 0.0f, 10.0f);
//            ImGui::Text("FPS");
//            ImGui::NewLine();
//            ImGui::SameLine(text_cursor);
//            ImGui::DragFloat("##fps", &context_.python.fps, 1.0f, 0.0f, 144.0f);
//            ImGui::Text("Visibility");
//            ImGui::NewLine();
//            ImGui::SameLine(text_cursor);
//            ImGui::DragFloat("##visibility", &context_.python.min_visibility, 0.1f, 0.0f, 1.0f);
//            ImGui::Text("Angle Adjustment");
//            ImGui::NewLine();
//            ImGui::SameLine(text_cursor);
//            ImGui::Checkbox("##is_angle_adjustment", &context_.python.is_angle_adjustment);
//            ImGui::Separator();
//            ImGui::Text("Model Complexity");
//            ImGui::NewLine();
//            ImGui::SameLine(text_cursor);
//            ImGui::DragInt("##model_complexity", &context_.python.model_complexity, 1, 0, 1);
//            ImGui::Text("Model Min Detection Confidence");
//            ImGui::NewLine();
//            ImGui::SameLine(text_cursor);
//            ImGui::DragFloat("##detection", &context_.python.min_detection_confidence, 0.1f, 0.1f, 1.0f);
//            if (ImGui::Button("OK"))
//            {
//                ImGui::CloseCurrentPopup();
//                context_.python.video_path = video_path;
//                context_.python.save_path = save_path;
//                context_.python.is_clicked_convert_btn = true;
//            }
//            ImGui::SameLine();
//            if (ImGui::Button("Close"))
//                ImGui::CloseCurrentPopup();
//            ImGui::EndPopup();
//        }
//        else
//        {
//			if (ImGuiFileDialog::Instance()->IsOpened())
//			{
//				context_.menu.is_dialog_open = true;
//			} else {
//				context_.menu.is_dialog_open = false;
//			}
//
//        }
//        ImGui::PopStyleColor();
//    }

void MainLayer::draw_scene(const std::string &title, Scene *scene)
{
	if (scene_layer_map_.find(title) == scene_layer_map_.end())
	{
		scene_layer_map_[title] = std::make_unique<SceneLayer>();
	}
	scene_layer_map_[title]->draw(title.c_str(), scene, context_);
}

void MainLayer::remove_entity_from_sequencer(std::shared_ptr<anim::Entity> entity){
	if(entity){
		timeline_layer_.remove_entity_from_sequencer(entity);
	}
}

void MainLayer::remove_camera_from_sequencer(std::shared_ptr<anim::Entity> entity){
	if(entity){
		timeline_layer_.remove_camera_from_sequencer(entity);
	}
}


void MainLayer::disable_entity_from_sequencer(std::shared_ptr<anim::Entity> entity){
	if(entity){
		timeline_layer_.disable_entity_from_sequencer(entity);
	}
}


void MainLayer::disable_camera_from_sequencer(std::shared_ptr<anim::Entity> entity){
	if(entity){
		timeline_layer_.disable_camera_from_sequencer(entity);
	}
}

void MainLayer::disable_light_from_sequencer(std::shared_ptr<anim::Entity> entity){
	if(entity){
		timeline_layer_.disable_light_from_sequencer(entity);
	}
}


void MainLayer::serialize_timeline(){
	timeline_layer_.serialize_timeline();
}

void MainLayer::draw_component_layer(Scene *scene)
{
	component_layer_.draw(context_.component, scene);
}

void MainLayer::draw_hierarchy_layer(Scene *scene)
{
	hierarchy_layer_.draw(scene, context_);
}
void MainLayer::init_timeline(Scene *scene){
	timeline_layer_.init(scene);
}
void MainLayer::draw_timeline(Scene *scene)
{
	timeline_layer_.draw(scene, context_);
}
void MainLayer::draw_resources(Scene *scene)
{
	resources_layer_.draw(scene, context_);
}

bool MainLayer::is_scene_layer_hovered(const std::string &title)
{
	if (scene_layer_map_.find(title) == scene_layer_map_.end())
	{
		return false;
	}
	return scene_layer_map_[title]->get_is_hovered();
}

bool MainLayer::is_scene_layer_focused(const std::string &title)
{
	if (scene_layer_map_.find(title) == scene_layer_map_.end())
	{
		return false;
	}
	return scene_layer_map_[title]->get_is_focused();
}
const UiContext &MainLayer::get_context() const
{
	return context_;
}
}
