#pragma once

#include "BlueprintNode.hpp"

#include <memory>
#include <optional>
#include <string>
#include <variant>

// Forward declarations
class BlueprintCanvas;
struct GLFWwindow;

class KeyReleaseCoreNode : public DataCoreNode {
public:
	explicit KeyReleaseCoreNode(UUID id);
	
	/**
	 * @brief Sets the node's data. This is the single entry point for updating state,
	 * used by both the UI and the deserializer.
	 */
	void set_data(std::optional<std::variant<Entity, std::string, long, float, bool>> data) override;
	
	/**
	 * @brief Gets the node's data for serialization.
	 */
	[[nodiscard]] std::optional<std::variant<Entity, std::string, long, float, bool>> get_data() const override;
	
	[[nodiscard]] int keycode() const { return mKeyCode; }
	[[nodiscard]] bool configured() const { return mConfigured; }
	[[nodiscard]] CorePin& output() { return mOutput; }
	
	void set_window(GLFWwindow* window) { mWindow = window; }
	
private:
	int mKeyCode; // Use `int` for GLFW key codes
	bool mConfigured;
	bool mWasPressed; // State tracking to detect the release event
	
	CorePin& mOutput;
	GLFWwindow* mWindow;
};

class KeyReleaseVisualNode : public VisualBlueprintNode {
public:
	KeyReleaseVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, KeyReleaseCoreNode& coreNode);
	
	bool keyboard_event(int key, int scancode, int action, int modifiers) override;
	
	void perform_layout(NVGcontext *ctx) override;
	
private:
	PassThroughButton& mActionButton;
	KeyReleaseCoreNode& mCoreNode;
	bool mListening;
};
