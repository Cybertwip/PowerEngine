/*
 * =====================================================================================
 *
 * Filename:  KeyReleaseNode.hpp
 *
 * Description:  Header file for the KeyReleaseNode class, which provides a
 * blueprint node that fires an event when a specific keyboard key is released.
 *
 * =====================================================================================
 */
#pragma once

#include "BlueprintNode.hpp"

#include <memory>
#include <optional>
#include <string>
#include <any>

// Forward declarations
class BlueprintCanvas;
struct GLFWwindow;

/**
 * @class KeyReleaseCoreNode
 * @brief The logical part of a "Key Release" node.
 *
 * This is an event node that triggers its output flow pin when a configured
 * key is released. It's a DataCoreNode to allow its configuration (the keycode)
 * to be saved and loaded.
 */
class KeyReleaseCoreNode : public DataCoreNode {
public:
	explicit KeyReleaseCoreNode(UUID id);
	
	/**
	 * @brief Sets the node's configured keycode from a std::any object.
	 * @param data An optional std::any containing the new keycode as a long.
	 */
	void set_data(std::optional<std::any> data) override;
	
	/**
	 * @brief Gets the node's configured keycode for serialization.
	 * @return An optional std::any containing the keycode as a long.
	 */
	std::optional<std::any> get_data() const override;
	
	/**
	 * @brief Called by the visual node when the configured key's state changes.
	 * @param action The GLFW key action (e.g., GLFW_PRESS, GLFW_RELEASE).
	 */
	void key_event_fired(int action);
	
	/**
	 * @brief Overrides the base class to define this node's evaluation logic.
	 * @return True if the key was just released, allowing the flow to continue.
	 */
	bool evaluate() override;
	
	[[nodiscard]] int keycode() const { return mKeyCode; }
	[[nodiscard]] bool configured() const { return mConfigured; }
	
	void set_window(GLFWwindow* window) { mWindow = window; }
	
private:
	int mKeyCode;
	bool mConfigured;
	bool mTriggered; // Used to signal the event pulse
	CorePin& mOutput;
	
	GLFWwindow* mWindow;
};

/**
 * @class KeyReleaseVisualNode
 * @brief The visual representation of a "Key Release" node.
 *
 * This class handles the UI for setting the key and listens for global
 * keyboard events to trigger the core node's logic.
 */
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
