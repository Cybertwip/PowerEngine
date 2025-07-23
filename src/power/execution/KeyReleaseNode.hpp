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
     * @brief Sets the node's data from UI or deserialization.
     */
    void set_data(std::optional<std::variant<Entity, std::string, long, float, bool>> data) override;
    
    /**
     * @brief Gets the node's data for serialization.
     */
    std::optional<std::variant<Entity, std::string, long, float, bool>> get_data() const override;
    
    /**
     * @brief Called by the visual node when the configured key's state changes.
     * @param action The GLFW key action (e.g., GLFW_PRESS, GLFW_RELEASE).
     */
    void key_event_fired(int action);

    /**
     * @brief Overrides the base class to define this node's evaluation logic.
     * @return True to continue the execution flow, false to stop it.
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