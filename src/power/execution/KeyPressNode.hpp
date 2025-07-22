#pragma once

#include "BlueprintNode.hpp"

#include <memory>
#include <optional>
#include <string>
#include <variant>

// Forward declarations
class BlueprintCanvas;
struct GLFWwindow;

class KeyPressCoreNode : public DataCoreNode {
public:
	explicit KeyPressCoreNode(UUID id);
	
	/**
	 * @brief Sets the node's data. This is the single entry point for updating state,
	 * used by both the UI and the deserializer.
	 */
	void set_data(std::optional<std::variant<Entity, std::string, long, float, bool>> data) override;
	
	/**
	 * @brief Gets the node's data for serialization.
	 */
	std::optional<std::variant<Entity, std::string, long, float, bool>> get_data() const override;
	
	[[nodiscard]] int keycode() const { return mKeyCode; }
	[[nodiscard]] bool configured() const { return mConfigured; }
	[[nodiscard]] CorePin& output() { return mOutput; }
	
	void set_window(GLFWwindow* window) { mWindow = window; }
	
private:
	int mKeyCode; // Use `int` for GLFW key codes for consistency.
	bool mConfigured;
	bool mTriggered;
	CorePin& mOutput;
	
	GLFWwindow* mWindow;
};

class KeyPressVisualNode : public VisualBlueprintNode {
public:
	KeyPressVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, KeyPressCoreNode& coreNode);
	
	bool keyboard_event(int key, int scancode, int action, int modifiers) override;
	
	void perform_layout(NVGcontext *ctx) override;
	
private:
	PassThroughButton& mActionButton;
	KeyPressCoreNode& mCoreNode;
	bool mListening;
};
