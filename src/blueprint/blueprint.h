#pragma once

#include "utilities/builders.h"
#include "utilities/widgets.h"

#include <imgui_node_editor.h>
#include <imgui_internal.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>
#include <filesystem>
#include <variant>
#include <optional>
#include <iostream>
#include <unordered_set>

#include "icons/icons.h"

#include "scene/scene.hpp"
#include "animation.h"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
static ImVec2 operator+(const ImVec2& a, const ImVec2& b) {
	return ImVec2(a.x + b.x, a.y + b.y);
}
static ImVec2 operator-(const ImVec2& a, const ImVec2& b) {
	return ImVec2(a.x - b.x, a.y - b.y);
}
#endif

class Scene;

namespace anim{
class RuntimeCinematicSequence;
}

namespace BluePrint {

static std::map<std::string, ImGuiKey> keyMap = {
	{"A", ImGuiKey_A}, {"B", ImGuiKey_B}, {"C", ImGuiKey_C}, {"D", ImGuiKey_D},
	{"E", ImGuiKey_E}, {"F", ImGuiKey_F}, {"G", ImGuiKey_G}, {"H", ImGuiKey_H},
	{"I", ImGuiKey_I}, {"J", ImGuiKey_J}, {"K", ImGuiKey_K}, {"L", ImGuiKey_L},
	{"M", ImGuiKey_M}, {"N", ImGuiKey_N}, {"O", ImGuiKey_O}, {"P", ImGuiKey_P},
	{"Q", ImGuiKey_Q}, {"R", ImGuiKey_R}, {"S", ImGuiKey_S}, {"T", ImGuiKey_T},
	{"U", ImGuiKey_U}, {"V", ImGuiKey_V}, {"W", ImGuiKey_W}, {"X", ImGuiKey_X},
	{"Y", ImGuiKey_Y}, {"Z", ImGuiKey_Z},
	{"0", ImGuiKey_0}, {"1", ImGuiKey_1}, {"2", ImGuiKey_2}, {"3", ImGuiKey_3},
	{"4", ImGuiKey_4}, {"5", ImGuiKey_5}, {"6", ImGuiKey_6}, {"7", ImGuiKey_7},
	{"8", ImGuiKey_8}, {"9", ImGuiKey_9},
	{"F1", ImGuiKey_F1}, {"F2", ImGuiKey_F2}, {"F3", ImGuiKey_F3}, {"F4", ImGuiKey_F4},
	{"F5", ImGuiKey_F5}, {"F6", ImGuiKey_F6}, {"F7", ImGuiKey_F7}, {"F8", ImGuiKey_F8},
	{"F9", ImGuiKey_F9}, {"F10", ImGuiKey_F10}, {"F11", ImGuiKey_F11}, {"F12", ImGuiKey_F12},
	{"UP", ImGuiKey_UpArrow}, {"DOWN", ImGuiKey_DownArrow}, {"LEFT", ImGuiKey_LeftArrow}, {"RIGHT", ImGuiKey_RightArrow},
	{"ENTER", ImGuiKey_Enter}, {"SPACE", ImGuiKey_Space}, {"ESC", ImGuiKey_Escape},
	{"TAB", ImGuiKey_Tab}, {"BACKSPACE", ImGuiKey_Backspace}, {"INSERT", ImGuiKey_Insert},
	{"DELETE", ImGuiKey_Delete}, {"PAGE_UP", ImGuiKey_PageUp}, {"PAGE_DOWN", ImGuiKey_PageDown},
	{"HOME", ImGuiKey_Home}, {"END", ImGuiKey_End}, {"CAPS_LOCK", ImGuiKey_CapsLock},
	{"SCROLL_LOCK", ImGuiKey_ScrollLock}, {"NUM_LOCK", ImGuiKey_NumLock},
	{"PRINT_SCREEN", ImGuiKey_PrintScreen}, {"PAUSE", ImGuiKey_Pause},
	{"LEFT_SHIFT", ImGuiKey_LeftShift}, {"RIGHT_SHIFT", ImGuiKey_RightShift},
	{"LEFT_CONTROL", ImGuiKey_LeftControl}, {"RIGHT_CONTROL", ImGuiKey_RightControl},
	{"LEFT_ALT", ImGuiKey_LeftAlt}, {"RIGHT_ALT", ImGuiKey_RightAlt},
	{"LEFT_SUPER", ImGuiKey_LeftSuper}, {"RIGHT_SUPER", ImGuiKey_RightSuper}
	// Note: ImGui may not directly support every key, especially system keys like "LEFT_SUPER", "RIGHT_SUPER".
	// This map is an approximation and may require adjustments based on your ImGui version and backend.
};


// Function to convert a string key name to its GLFW key code
static int StringToKeyCode(const std::string& keyName) {
	auto it = keyMap.find(keyName);
	if (it != keyMap.end()) {
		return it->second;
	} else {
		// Key not found, return -1
		return -1;
	}
}


static inline ImRect ImGui_GetItemRect()
{
	return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

static inline ImRect ImRect_Expanded(const ImRect& rect, float x, float y)
{
	auto result = rect;
	result.Min.x -= x;
	result.Min.y -= y;
	result.Max.x += x;
	result.Max.y += y;
	return result;
}

namespace ed = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;

using namespace ax;

using ax::Widgets::IconType;

static ed::EditorContext* m_Editor = nullptr;

enum class PinType
{
	Flow,
	Bool,
	Int,
	Float,
	String,
	Object,
	Function,
	Delegate,
};

enum class PinSubType {
	None,
	Actor,
	Light,
	Camera,
	Animation,
	Sequence,
	Composition
};


enum class PinKind
{
	Output,
	Input
};

struct Node;

struct Entity {
	int Id;
	std::optional<std::variant<std::string, int, float, bool>> payload;
};

struct Pin
{
	ed::PinId   ID;
	ed::NodeId NodeID; // Added NodeId member variable
	Node*     Node;
	std::string Name;
	PinType     Type;
	PinSubType	SubType;
	PinKind     Kind;
	
	Pin(int id, ed::NodeId nodeId, const char* name, PinType type, PinSubType subType = PinSubType::None):
	ID(id), NodeID(nodeId), Node(nullptr), Name(name), Type(type), SubType(subType), Kind(PinKind::Input)
	{
		if(Type == PinType::Bool){
			Data = true;
		} else if(Type == PinType::String){
			Data = "";
		} else if(Type == PinType::String){
			Data = "";
		} else if(Type == PinType::Float){
			Data = {};
		} else if(Type == PinType::Flow){
			Data = true;
		}
	}
	
	bool CanFlow = false;
	
	std::optional<std::variant<Entity, std::string, int, float, bool>> Data;
};

struct Node
{
	ed::NodeId ID;
	std::string Name;
	std::vector<Pin> Inputs;
	std::vector<Pin> Outputs;
	ImColor Color;
	ImVec2 Size;
	
	std::string State;
	std::string SavedState;
	
	Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)):
	ID(id), Name(name), Color(color), Size(0, 0), RootNode(false)
	{
	}
	
	void ResetFlow(){
		for(auto& input : Inputs){
			if(input.Type == PinType::Flow){
				input.CanFlow = false;
			}
		}
		for(auto& output : Outputs){
			if(output.Type == PinType::Flow){
				output.CanFlow = false;
			}
		}
	}
	
	std::function<void()> OnLinked;
	std::function<void()> Evaluate;
	
	bool RootNode;
};

struct Link
{
	ed::LinkId ID;
	
	ed::PinId StartPinID;
	ed::PinId EndPinID;
	
	ImColor Color;
	
	Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId):
	ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
	{
	}
};

struct NodeIdLess
{
	bool operator()(const ed::NodeId& lhs, const ed::NodeId& rhs) const
	{
		return lhs.AsPointer() < rhs.AsPointer();
	}
};

struct NodeProcessor
{
	Scene& _scene;
	
	NodeProcessor(Scene& scene) : _scene(scene) {
		
	}
	
	int GetNextId()
	{
		return m_NextId++;
	}
	
	ed::LinkId GetNextLinkId()
	{
		return ed::LinkId(GetNextId());
	}
	
	void TouchNode(ed::NodeId id)
	{
		m_NodeTouchTime[id] = m_TouchTime;
	}
	
	float GetTouchProgress(ed::NodeId id)
	{
		auto it = m_NodeTouchTime.find(id);
		if (it != m_NodeTouchTime.end() && it->second > 0.0f)
			return (m_TouchTime - it->second) / m_TouchTime;
		else
			return 0.0f;
	}
	
	void UpdateTouch()
	{
		const auto deltaTime = ImGui::GetIO().DeltaTime;
		for (auto& entry : m_NodeTouchTime)
		{
			if (entry.second > 0.0f)
				entry.second -= deltaTime;
		}
	}
	
	Node* FindNode(ed::NodeId id)
	{
		for (auto& node : m_Nodes)
			if (node->ID == id)
				return node.get();
		
		return nullptr;
	}
	
	Link* FindLink(ed::LinkId id)
	{
		for (auto& link : m_Links)
			if (link->ID == id)
				return link.get();
		
		return nullptr;
	}
	
	std::vector<Link*> GetNodeLinks(Node* node) {
		std::vector<Link*> associatedLinks;
		if (!node)
			return associatedLinks;
		
		// Iterate through all links
		for (const auto& link : m_Links) {
			
			bool owning = false;
			
			if(FindPin(link->StartPinID)){
				if(FindPin(link->StartPinID)->NodeID == node->ID){
					owning = true;
				}
			}
			
			if(FindPin(link->EndPinID)){
				if(FindPin(link->EndPinID)->NodeID == node->ID){
					owning = true;
				}
			}
			
			if(owning){
				associatedLinks.push_back(link.get());
			}
		}
		
		return associatedLinks;
	}



	Pin* FindPin(ed::PinId id)
	{
		if (!id)
			return nullptr;
		
		for (auto& node : m_Nodes)
		{
			for (auto& pin : node->Inputs)
				if (pin.ID == id)
					return &pin;
			
			for (auto& pin : node->Outputs)
				if (pin.ID == id)
					return &pin;
		}
		
		return nullptr;
	}
	
	bool IsPinLinked(ed::PinId id)
	{
		if (!id)
			return false;
		
		for (auto& link : m_Links)
			if (link->StartPinID == id || link->EndPinID == id)
				return true;
		
		return false;
	}
	
	bool CanCreateLink(Pin* a, Pin* b)
	{
		if (!a || !b || a == b || a->Kind == b->Kind || a->Type != b->Type || a->Node == b->Node || (b->Type != PinType::Flow && IsPinLinked(b->ID)))
			return false;
		
		return true;
	}
	
	void BuildNode(Node* node)
	{
		for (auto& input : node->Inputs)
		{
			input.Node = node;
			input.Kind = PinKind::Input;
		}
		
		for (auto& output : node->Outputs)
		{
			output.Node = node;
			output.Kind = PinKind::Output;
		}
	}
	
	Node* SpawnInputActionNode(const std::string& keyString, int keyCode)
	{
		auto label = "Input Action " + keyString;
		m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), label.c_str(), ImColor(255, 128, 128)));
		auto& node = *m_Nodes.back();
		
		node.RootNode = true;
		
		node.Outputs.emplace_back(GetNextId(), node.ID, "Pressed", PinType::Flow);
		node.Outputs.emplace_back(GetNextId(), node.ID, "Released", PinType::Flow);
		
		node.Evaluate = [&node, keyCode](){
			if(keyCode != -1){
				if(ImGui::GetIO().KeysDown[ImGui::GetKeyIndex(keyCode)]){
					node.Outputs[0].CanFlow = true;
				} else if(!ImGui::GetIO().KeysDown[ImGui::GetKeyIndex(keyCode)]){
					if(node.Outputs[0].CanFlow){
						node.Outputs[0].CanFlow = false;
						node.Outputs[1].CanFlow = true;
					} else {
						node.Outputs[1].CanFlow = false;
					}
				}
			} else {
				node.Outputs[0].CanFlow = false;
				node.Outputs[1].CanFlow = false;
			}
		};
		
		BuildNode(&node);
		
		return &node;
	}
	
	Node* SpawnBranchNode()
	{
		m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), "Branch"));
		auto& node = *m_Nodes.back();
		
		node.Inputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
		node.Inputs.emplace_back(GetNextId(), node.ID, "Condition", PinType::Bool);
		node.Outputs.emplace_back(GetNextId(), node.ID, "True", PinType::Flow);
		node.Outputs.emplace_back(GetNextId(), node.ID, "False", PinType::Flow);
		
		node.OnLinked = [&node](){
			node.Inputs[1].CanFlow = true;
		};
		
		node.Evaluate = [&node](){
			auto data = node.Inputs[1].Data;
			if(data.has_value()){
				if(std::get<bool>(*data)){
					node.Outputs[0].CanFlow = true;
					node.Outputs[1].CanFlow = false;
				} else {
					node.Outputs[0].CanFlow = false;
					node.Outputs[1].CanFlow = true;
				}
			}
			
			if(!node.Inputs[0].CanFlow){
				node.Outputs[0].CanFlow = false;
				node.Outputs[1].CanFlow = false;
			}
		};
		
		BuildNode(&node);
		
		return &node;
	}
	
	Node* SpawnDoNNode()
	{
		m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), "Do N"));
		auto& node = *m_Nodes.back();
		
		node.Inputs.emplace_back(GetNextId(), node.ID, "Enter", PinType::Flow);
		node.Inputs.emplace_back(GetNextId(), node.ID, "N", PinType::Int);
		node.Inputs.emplace_back(GetNextId(), node.ID, "Reset", PinType::Flow);
		node.Outputs.emplace_back(GetNextId(), node.ID, "Exit", PinType::Flow);
		node.Outputs.emplace_back(GetNextId(), node.ID, "Counter", PinType::Int);
		
		BuildNode(&node);
		
		return &node;
	}
	
	Node* SpawnGetActorNode(int actorId);
	Node* SpawnGetAnimationNode(int animationId);
	Node* SpawnGetActorPositionNode();
	Node* SpawnSetActorPositionNode();

	Node* SpawnGetActorRotationNode();
	Node* SpawnSetActorRotationNode();

	Node* SpawnPlayAnimationNode();
	Node* SpawnAddNode();
	
	Node* SpawnPrintStringNode()
	{
		m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), "Print String"));
		auto& node = *m_Nodes.back();
		node.Inputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
		node.Inputs.emplace_back(GetNextId(), node.ID, "In String", PinType::String);
		node.Outputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
		
		node.OnLinked = [&node](){
			node.Outputs[0].CanFlow = true;
		};
		
		node.Evaluate = [&node](){
			if(node.Inputs[0].CanFlow){
				auto& data = node.Inputs[1].Data;
				if(data.has_value()){
					std::cout << std::get<std::string>(*data) << std::endl;
				}
			}
			
			node.Inputs[1].CanFlow = node.Inputs[0].CanFlow;
		};
		
		BuildNode(&node);
		
		return &node;
	}
	
	Node* SpawnMessageNode()
	{
		m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), "", ImColor(128, 195, 248)));
		auto& node = *m_Nodes.back();
		
		node.Outputs.emplace_back(GetNextId(), node.ID, "Value", PinType::String);
		
		node.Outputs[0].Data = "Hello Power";
		
		node.OnLinked = [&node](){
			node.Outputs[0].CanFlow = true;
		};
		
		BuildNode(&node);
		
		return &node;
	}
	
	
	Node* SpawnConstantNode()
	{
		m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), "", ImColor(128, 195, 248)));
		auto& node = *m_Nodes.back();
		
		node.Outputs.emplace_back(GetNextId(), node.ID, "Value", PinType::Float);
		
		node.Outputs[0].Data = 0.0f;
		
		node.OnLinked = [&node](){
			node.Outputs[0].CanFlow = true;
		};
		
		BuildNode(&node);
		
		return &node;
	}
	
	Node* SpawnSetTimerNode()
	{
		m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), "Set Timer", ImColor(128, 195, 248)));
		auto& node = *m_Nodes.back();
		
		node.Inputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
		node.Inputs.emplace_back(GetNextId(), node.ID, "Object", PinType::Object);
		node.Inputs.emplace_back(GetNextId(), node.ID, "Function Name", PinType::Function);
		node.Inputs.emplace_back(GetNextId(), node.ID, "Time", PinType::Float);
		node.Inputs.emplace_back(GetNextId(), node.ID, "Looping", PinType::Bool);
		node.Outputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
		
		BuildNode(&node);
		
		return &node;
	}
	
	Node* SpawnLessNode()
	{
		m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), "<", ImColor(128, 195, 248)));
		auto& node = *m_Nodes.back();
		
		node.Inputs.emplace_back(GetNextId(), node.ID, "", PinType::Float);
		node.Inputs.emplace_back(GetNextId(), node.ID, "", PinType::Float);
		node.Outputs.emplace_back(GetNextId(), node.ID, "", PinType::Float);
		
		BuildNode(&node);
		
		return &node;
	}
	
	void BuildNodes()
	{
		for (auto& node : m_Nodes)
			BuildNode(node.get());
	}
	
	static void Setup()
	{
		//		ed::Config config;
		//
		//		config.SettingsFile = "Blueprints.json";
		//
		//		config.UserPointer = this;
		//
		//		config.LoadNodeSettings = [](ed::NodeId nodeId, char* data, void* userPointer) -> size_t
		//		{
		//			auto self = static_cast<NodeProcessor*>(userPointer);
		//
		//			auto node = self->FindNode(nodeId);
		//			if (!node)
		//				return 0;
		//
		//			if (data != nullptr)
		//				memcpy(data, node->State.data(), node->State.size());
		//			return node->State.size();
		//		};
		//
		//		config.SaveNodeSettings = [](ed::NodeId nodeId, const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer) -> bool
		//		{
		//			auto self = static_cast<NodeProcessor*>(userPointer);
		//
		//			auto node = self->FindNode(nodeId);
		//			if (!node)
		//				return false;
		//
		//			node->State.assign(data, size);
		//
		//			self->TouchNode(nodeId);
		//
		//			return true;
		//		};
		//
		m_Editor = ed::CreateEditor(nullptr);
		//		ed::SetCurrentEditor(m_Editor);
		//
		//		Node* node;
		//
		//		// Starting X and Y positions
		//		int startX = -348;
		//		int startY = 220;
		//		int yOffset = 100; // Vertical spacing between nodes
		//
		//		node = SpawnInputActionNode();
		//		ed::SetNodePosition(node->ID, ImVec2(startX, startY));
		//
		//		node = SpawnMessageNode();
		//		ed::SetNodePosition(node->ID, ImVec2(startX, startY += yOffset)); // Increment Y position
		//
		//		node = SpawnGetActorNode();
		//		ed::SetNodePosition(node->ID, ImVec2(startX, startY += yOffset));
		//
		//		// Assuming these nodes should be spaced uniquely since they were at the same position before
		//		node = SpawnMessageNode();
		//		ed::SetNodePosition(node->ID, ImVec2(startX, startY += yOffset));
		//
		//		node = SpawnPlaySequenceNode();
		//		ed::SetNodePosition(node->ID, ImVec2(startX, startY += yOffset));
		//
		//		node = SpawnMessageNode(); // Another message node, positioned further down
		//		ed::SetNodePosition(node->ID, ImVec2(startX, startY += yOffset));
		//
		//		ed::NavigateToContent();
		//
		//		BuildNodes();
	}
	
	static void Teardown()
	{
		
		if (m_Editor)
		{
			ed::DestroyEditor(m_Editor);
			m_Editor = nullptr;
		}
	}
	
	ImColor GetIconColor(PinType type)
	{
		switch (type)
		{
			default:
			case PinType::Flow:     return ImColor(255, 255, 255);
			case PinType::Bool:     return ImColor(220,  48,  48);
			case PinType::Int:      return ImColor( 68, 201, 156);
			case PinType::Float:    return ImColor(147, 226,  74);
			case PinType::String:   return ImColor(124,  21, 153);
			case PinType::Object:   return ImColor( 51, 150, 215);
			case PinType::Function: return ImColor(218,   0, 183);
			case PinType::Delegate: return ImColor(255,  48,  48);
		}
	};
	
	void DrawPinIcon(const Pin& pin, bool connected, int alpha)
	{
		IconType iconType;
		ImColor  color = GetIconColor(pin.Type);
		color.Value.w = alpha / 255.0f;
		switch (pin.Type)
		{
			case PinType::Flow:     iconType = IconType::Flow;   break;
			case PinType::Bool:     iconType = IconType::Circle; break;
			case PinType::Int:      iconType = IconType::Circle; break;
			case PinType::Float:    iconType = IconType::Circle; break;
			case PinType::String:   iconType = IconType::Circle; break;
			case PinType::Object:   iconType = IconType::Circle; break;
			case PinType::Function: iconType = IconType::Circle; break;
			case PinType::Delegate: iconType = IconType::Square; break;
			default:
				return;
		}
		
		ax::Widgets::Icon(ImVec2(static_cast<float>(m_PinIconSize), static_cast<float>(m_PinIconSize)), iconType, connected, color, ImColor(32, 32, 32, alpha));
	};
	
	void OnFrame(float deltaTime)
	{
		ed::SetCurrentEditor(m_Editor);
		
		UpdateTouch();
		
		auto& io = ImGui::GetIO();
		
		std::string icon = ICON_MD_PLAY_ARROW;
		
		icon += "Run";
		
		if(ImGui::Button(icon.c_str())){
			s_Running = !s_Running;
		}
		
		ed::SetCurrentEditor(m_Editor);
		
		static ed::NodeId contextNodeId      = 0;
		static ed::LinkId contextLinkId      = 0;
		static ed::PinId  contextPinId       = 0;
		static bool createNewNode  = false;
		static Pin* newNodeLinkPin = nullptr;
		static Pin* newLinkPin     = nullptr;
		
		ed::Begin("Node editor");
		{
			auto cursorTopLeft = ImGui::GetCursorScreenPos();
			
			util::BlueprintNodeBuilder builder(nullptr, 200, 200);
			
			for (auto& node : m_Nodes)
			{
				bool hasOutputDelegates = false;
				for (auto& output : node->Outputs)
					if (output.Type == PinType::Delegate)
						hasOutputDelegates = true;
				
				builder.Begin(node->ID);
				builder.Header(node->Color);
				ImGui::Spring(0);
				ImGui::TextUnformatted(node->Name.c_str());
				ImGui::Spring(1);
				ImGui::Dummy(ImVec2(0, 28));
				if (hasOutputDelegates)
				{
					ImGui::BeginVertical("delegates", ImVec2(0, 28));
					ImGui::Spring(1, 0);
					for (auto& output : node->Outputs)
					{
						if (output.Type != PinType::Delegate)
							continue;
						
						auto alpha = ImGui::GetStyle().Alpha;
						if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
							alpha = alpha * (48.0f / 255.0f);
						
						ed::BeginPin(output.ID, ed::PinKind::Output);
						ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
						ed::PinPivotSize(ImVec2(0, 0));
						ImGui::BeginHorizontal(output.ID.AsPointer());
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
						if (!output.Name.empty())
						{
							ImGui::TextUnformatted(output.Name.c_str());
							ImGui::Spring(0);
						}
						DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
						ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
						ImGui::EndHorizontal();
						ImGui::PopStyleVar();
						ed::EndPin();
						
						//DrawItemRect(ImColor(255, 0, 0));
					}
					ImGui::Spring(1, 0);
					ImGui::EndVertical();
					ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
				}
				else
					ImGui::Spring(0);
				builder.EndHeader();
				
				for (auto& input : node->Inputs)
				{
					auto alpha = ImGui::GetStyle().Alpha;
					if (newLinkPin && !CanCreateLink(newLinkPin, &input) && &input != newLinkPin)
						alpha = alpha * (48.0f / 255.0f);
					
					builder.Input(input.ID);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
					DrawPinIcon(input, IsPinLinked(input.ID), (int)(alpha * 255));
					ImGui::Spring(0);
					if (!input.Name.empty())
					{
						ImGui::TextUnformatted(input.Name.c_str());
						ImGui::Spring(0);
					}
					if (input.Type == PinType::Bool)
					{
						bool check = true;
						if(input.Data.has_value()){
							check = std::get<bool>(*input.Data);
						}
						
						if(!IsPinLinked(input.ID)){
							ImGui::Checkbox("", &check);
						}
						
						input.Data = check;
						
						ImGui::Spring(0);
					}
					
					if (input.Type == PinType::Float)
					{
						float buffer = 0.0f;
						
						if(input.Data.has_value()){
							buffer = std::get<float>(*input.Data);
						}
						if(!IsPinLinked(input.ID)){
							ImGui::PushItemWidth(100.0f);
							if (ImGui::DragFloat("##edit", &buffer)) {
								// Update output.Data with the modified string
								input.Data = buffer;
							}
							ImGui::PopItemWidth();
						}
						ImGui::Spring(0);
					}
					ImGui::PopStyleVar();
					builder.EndInput();
				}
				
				for (auto& output : node->Outputs)
				{
					if (output.Type == PinType::Delegate)
						continue;
					
					auto alpha = ImGui::GetStyle().Alpha;
					if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
						alpha = alpha * (48.0f / 255.0f);
					
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
					builder.Output(output.ID);
					if (output.Type == PinType::String)
					{
						static bool wasActive = false;
						
						std::string buffer = "Message";
						
						if(output.Data.has_value()){
							buffer = std::get<std::string>(*output.Data);
						}
						
						std::vector<char> charBuffer(buffer.begin(), buffer.end());
						charBuffer.resize(256); // Resize to provide extra space for input
						
						ImGui::PushItemWidth(100.0f);
						if (ImGui::InputText("##edit", charBuffer.data(), charBuffer.size())) {
							// Update the buffer with the potentially modified string
							buffer.assign(charBuffer.data());
							
							// Update output.Data with the modified string
							output.Data = buffer;
						}
						ImGui::PopItemWidth();
						
						if (ImGui::IsItemActive() && !wasActive)
						{
							ed::EnableShortcuts(false);
							wasActive = true;
						}
						else if (!ImGui::IsItemActive() && wasActive)
						{
							ed::EnableShortcuts(true);
							wasActive = false;
						}
						ImGui::Spring(0);
					}
					if (!output.Name.empty())
					{
						ImGui::Spring(0);
						ImGui::TextUnformatted(output.Name.c_str());
					}
					ImGui::Spring(0);
					DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
					ImGui::PopStyleVar();
					builder.EndOutput();
				}
				
				builder.End();
			}
			
			for (auto& link : m_Links)
				ed::Link(link->ID, link->StartPinID, link->EndPinID, link->Color, 2.0f);
			
			if (!createNewNode)
			{
				if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
				{
					auto showLabel = [](const char* label, ImColor color)
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
						auto size = ImGui::CalcTextSize(label);
						
						auto padding = ImGui::GetStyle().FramePadding;
						auto spacing = ImGui::GetStyle().ItemSpacing;
						
						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));
						
						auto rectMin = ImGui::GetCursorScreenPos() - padding;
						auto rectMax = ImGui::GetCursorScreenPos() + size + padding;
						
						auto drawList = ImGui::GetWindowDrawList();
						drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
						ImGui::TextUnformatted(label);
					};
					
					ed::PinId startPinId = 0, endPinId = 0;
					if (ed::QueryNewLink(&startPinId, &endPinId))
					{
						auto startPin = FindPin(startPinId);
						auto endPin   = FindPin(endPinId);
						
						newLinkPin = startPin ? startPin : endPin;
						
						if (startPin->Kind == PinKind::Input)
						{
							std::swap(startPin, endPin);
							std::swap(startPinId, endPinId);
						}
						
						if (startPin && endPin)
						{
							if (endPin == startPin)
							{
								ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
							}
							else if (endPin->Kind == startPin->Kind)
							{
								showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
								ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
							}
							else if (endPin->Type != startPin->Type)
							{
								showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
								ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
							} else if(endPin->Type == startPin->Type && endPin->Type == PinType::Object && endPin->SubType != startPin->SubType){
								showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
								ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
							}
							else if(!CanCreateLink(startPin, endPin)){
								ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
							}
							else
							{
								showLabel("+ Create Link", ImColor(32, 45, 32, 180));
								if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
								{
									m_Links.emplace_back(std::make_unique<Link>(GetNextId(), startPinId, endPinId));
									m_Links.back()->Color = GetIconColor(startPin->Type);
									
									if(FindPin(startPin->ID)->Node->OnLinked){
										FindPin(startPin->ID)->Node->OnLinked();
									}
									
									if(FindPin(endPin->ID)->Node->OnLinked){
										FindPin(endPin->ID)->Node->OnLinked();
									}
								}
							}
						}
					}
					
					ed::PinId pinId = 0;
					if (ed::QueryNewNode(&pinId))
					{
						newLinkPin = FindPin(pinId);
						if (newLinkPin)
							showLabel("+ Create Node", ImColor(32, 45, 32, 180));
						
						if (ed::AcceptNewItem())
						{
							createNewNode  = true;
							newNodeLinkPin = FindPin(pinId);
							newLinkPin = nullptr;
							ed::Suspend();
							ImGui::OpenPopup("Create New Node");
							ed::Resume();
						}
					}
				}
				else
					newLinkPin = nullptr;
				
				ed::EndCreate();
				
				if (ed::BeginDelete())
				{
					ed::NodeId nodeId = 0;
					while (ed::QueryDeletedNode(&nodeId))
					{
						if (ed::AcceptDeletedItem())
						{
							auto id = std::find_if(m_Nodes.begin(), m_Nodes.end(), [nodeId](auto& node) { return node->ID == nodeId; });
							if (id != m_Nodes.end())
								m_Nodes.erase(id);
						}
					}
					
					ed::LinkId linkId = 0;
					while (ed::QueryDeletedLink(&linkId))
					{
						if (ed::AcceptDeletedItem())
						{
							auto id = std::find_if(m_Links.begin(), m_Links.end(), [linkId](auto& link) { return link->ID == linkId; });
							if (id != m_Links.end()){
								
								if(FindPin((*id)->StartPinID)){
									FindPin((*id)->StartPinID)->CanFlow = false;
								}
								
								if(FindPin((*id)->EndPinID)){
									FindPin((*id)->EndPinID)->CanFlow = false;
								}
								
								if(FindPin((*id)->EndPinID)){
									if(FindPin((*id)->EndPinID)->Type == PinType::Flow){
										FindPin((*id)->EndPinID)->Node->ResetFlow();
									}
								}
								
								m_Links.erase(id);
							}
						}
					}
				}
				ed::EndDelete();
			}
			
			ImGui::SetCursorScreenPos(cursorTopLeft);
		}
		
		if (ImGui::BeginDragDropTarget())
		{
			Node* node = nullptr;
			auto nodeSpawnPosition = ImGui::GetMousePos();
			
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ACTOR")) {
				const char* id_data = reinterpret_cast<const char*>(payload->Data);
				
				int actor_id = std::atoi(id_data);
				
				node = SpawnGetActorNode(actor_id);
				
				ImGui::EndDragDropTarget();
			}
			
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FBX_ANIMATION_PATH")) {
				const char* sequencePath = reinterpret_cast<const char*>(payload->Data);
				
				auto resources = _scene.get_mutable_shared_resources();
				
				resources->import_model(sequencePath);
				
				auto& animationSet = resources->getAnimationSet(sequencePath);
				
				resources->add_animations(animationSet.animations);
				
				auto animation = animationSet.animations[0];
				
				int animation_id = animation->get_id();
				
				node = SpawnGetAnimationNode(animation_id);
				
				ImGui::EndDragDropTarget();
			}
			
			if (node)
			{
				BuildNodes();
				
				createNewNode = false;
				
				ed::SetNodePosition(node->ID, nodeSpawnPosition);
				
				if (auto startPin = newNodeLinkPin)
				{
					auto& pins = startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs;
					
					for (auto& pin : pins)
					{
						if (CanCreateLink(startPin, &pin))
						{
							auto endPin = &pin;
							if (startPin->Kind == PinKind::Input)
								std::swap(startPin, endPin);
							
							m_Links.emplace_back(std::make_unique<Link>(GetNextId(), startPin->ID, endPin->ID));
							m_Links.back()->Color = GetIconColor(startPin->Type);
							
							if(FindPin(startPin->ID)->Node->OnLinked){
								FindPin(startPin->ID)->Node->OnLinked();
							}
							
							if(FindPin(endPin->ID)->Node->OnLinked){
								FindPin(endPin->ID)->Node->OnLinked();
							}
							break;
						}
					}
				}
			}
		}
		
		static bool popupPositionSet = false;
		static ImVec2 openPopupPosition = ImVec2(0, 0);
		
		if(!popupPositionSet){
			openPopupPosition = ImGui::GetMousePos();
			popupPositionSet = true;
		}
		ed::Suspend();
		if (ed::ShowBackgroundContextMenu())
		{
			ImGui::OpenPopup("Create New Node");
			newNodeLinkPin = nullptr;
		}
		ed::Resume();
		
		ed::Suspend();
		
		static ImVec2 longestKeySize = ImGui::CalcTextSize("Key RIGHT_CONTROL");
		ImGui::SetNextWindowSize(ImVec2(longestKeySize.x * 1.35f, 0));
		
		if (ImGui::BeginPopup("Create New Node"))
		{
			auto newNodePostion = openPopupPosition;
			float contentWidth = ImGui::GetContentRegionAvailWidth();
			newNodePostion.x += longestKeySize.x * 1.5f;
			
			Node* node = nullptr;
			if (ImGui::CollapsingHeader("Keyboard Input")) {
				ImVec2 childSize(longestKeySize.x * 1.25f, ImGui::GetTextLineHeightWithSpacing() * 5);
				// Calculate the width of the child content
				float contentWidth = ImGui::GetContentRegionAvailWidth();
				
				// Adjust the width of the child content if it exceeds the available space
				if (contentWidth < childSize.x)
					childSize.x = contentWidth;
				
				ImGui::BeginChild("InputActionsScrolling", childSize, true);
				
				for (auto& mapping : keyMap) {
					auto keyString = "Key " + mapping.first;
					auto keyCode = mapping.second;
					
					if (ImGui::MenuItem(keyString.c_str())) {
						node = SpawnInputActionNode(keyString, keyCode);
						ImGui::CloseCurrentPopup();
						break;
					}
				}
				ImGui::EndChild();
			}
			
			// Group: Actor
			if (ImGui::CollapsingHeader("Actor")) {
				if (ImGui::MenuItem("Get Position"))
					node = SpawnGetActorPositionNode();
				if (ImGui::MenuItem("Set Position"))
					node = SpawnSetActorPositionNode();
				if (ImGui::MenuItem("Get Rotation"))
					node = SpawnGetActorRotationNode();
				if (ImGui::MenuItem("Set Rotation"))
					node = SpawnSetActorRotationNode();
				if (ImGui::MenuItem("Play Animation"))
					node = SpawnPlayAnimationNode();
			}
			
			// Group: Flow
			if (ImGui::CollapsingHeader("Flow")) {
				if (ImGui::MenuItem("Branch"))
					node = SpawnBranchNode();
				if (ImGui::MenuItem("Add"))
					node = SpawnAddNode();
			}
			
			// Group: Values
			if (ImGui::CollapsingHeader("Values")) {
				if (ImGui::MenuItem("Constant"))
					node = SpawnConstantNode();
				if (ImGui::MenuItem("String"))
					node = SpawnMessageNode();
			}
			
			// Group: Debug
			if (ImGui::CollapsingHeader("Debug")) {
				if (ImGui::MenuItem("Print"))
					node = SpawnPrintStringNode();
			}
			
			if (node)
			{
				BuildNodes();
				
				createNewNode = false;
				
				ed::SetNodePosition(node->ID, newNodePostion);
				
				if (auto startPin = newNodeLinkPin)
				{
					auto& pins = startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs;
					
					for (auto& pin : pins)
					{
						if (CanCreateLink(startPin, &pin))
						{
							auto endPin = &pin;
							if (startPin->Kind == PinKind::Input)
								std::swap(startPin, endPin);
							
							m_Links.emplace_back(std::make_unique<Link>(GetNextId(), startPin->ID, endPin->ID));
							m_Links.back()->Color = GetIconColor(startPin->Type);
							
							if(FindPin(startPin->ID)->Node->OnLinked){
								FindPin(startPin->ID)->Node->OnLinked();
							}
							
							if(FindPin(endPin->ID)->Node->OnLinked){
								FindPin(endPin->ID)->Node->OnLinked();
							}
							
							break;
						}
					}
				}
				
				popupPositionSet = false;
			}
			
			ImGui::EndPopup();
		} else{
			popupPositionSet = false;
			createNewNode = false;
		}
		ed::Resume();
		ed::End();
	}
	
	void Update(){
		m_SequenceStack.clear();
		
		if(s_Running){
			std::unordered_set<Node*> evaluatedNodes;
			
			for (auto& link : m_Links)
			{
				auto outPin = FindPin(link->StartPinID);
				
				if(evaluatedNodes.find(outPin->Node) == evaluatedNodes.end()){
					if(outPin->Node->Evaluate){
						outPin->Node->Evaluate();
						
						evaluatedNodes.insert(outPin->Node);
					}
				}
			}
			
			for (auto& link : m_Links)
			{
				auto inPin = FindPin(link->EndPinID);
				if(evaluatedNodes.find(inPin->Node) == evaluatedNodes.end()){
					if(inPin->Node->Evaluate){
						inPin->Node->Evaluate();
						evaluatedNodes.insert(inPin->Node);
					}
				}
			}
			
			for (auto& link : m_Links) {
				auto inPin = FindPin(link->EndPinID);
				
				if (inPin && inPin->Type == PinType::Flow) {
					bool canFlow = false;  // Initialize to false
					
					for (auto& otherLink : m_Links) {
						auto otherPin = FindPin(otherLink->EndPinID);

						if (otherPin == inPin) {
							auto startPin = FindPin(otherLink->StartPinID);
							if (startPin && startPin->Type == PinType::Flow) {
								canFlow = canFlow || startPin->CanFlow; // If any start pin can flow, set canFlow to true
							}
						}
					}
					
					inPin->CanFlow = canFlow;
				}
			}

			for (auto& link : m_Links)
			{
				auto outPin = FindPin(link->StartPinID);
				auto inPin = FindPin(link->EndPinID);
				
				inPin->Data = outPin->Data;
				
				if(inPin->Type == PinType::Flow && outPin->Type == PinType::Flow){
					if(outPin->CanFlow && inPin->CanFlow){
						ed::Flow(link->ID);
					}
				} else {
					if(inPin->CanFlow && outPin->CanFlow){
						ed::Flow(link->ID);
					}
				}
			}
		}
		
	}

	void OnActorRemoved(int entityId) {
		for (auto it = m_Nodes.begin(); it != m_Nodes.end(); ) {
			auto& node = *it;
			
			if(node->Evaluate){
				node->Evaluate();
			}
			
			// Check if any output pin of the node contains data with matching Entity ID
			bool containsEntity = std::any_of(node->Outputs.begin(), node->Outputs.end(), [&](const Pin& output) {
				return output.Kind == PinKind::Output && output.SubType == PinSubType::Actor &&
				output.Data.has_value() &&
				std::visit([&](auto&& arg) {
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, Entity>) {
						return arg.Id == entityId;
					}
					return false;
				}, output.Data.value());
			});
			
			if (containsEntity) {
				// Get all links associated with the node
				auto links = GetNodeLinks(node.get());
				
				// Erase the node
				it = m_Nodes.erase(it);
				
				// Erase all links associated with the node
				for (auto link : links) {
					m_Links.erase(std::remove_if(m_Links.begin(), m_Links.end(), [&](const auto& l) {
						return l.get() == link;
					}), m_Links.end());
				}
			} else {
				++it;
			}
		}
	}

	void OnSequenceRemoved(const std::string& sequencePath){
		
	}
	
public:
	static bool Evaluating() {
		return s_Running;
	}
	
	const std::vector<std::shared_ptr<anim::RuntimeCinematicSequence>>& GetSequenceStack() const {
		return m_SequenceStack;
	}
	
private:
	int                  m_NextId = 1;
	const int            m_PinIconSize = 24;
	std::vector<std::unique_ptr<Node>>    m_Nodes;
	std::vector<std::unique_ptr<Link>>    m_Links;
	const float          m_TouchTime = 1.0f;
	std::map<ed::NodeId, float, NodeIdLess> m_NodeTouchTime;
	
	static bool s_Running;
	
	std::vector<std::shared_ptr<anim::RuntimeCinematicSequence>> m_SequenceStack;
};
}
