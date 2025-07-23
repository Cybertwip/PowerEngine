#include "KeyPressNode.hpp"

#include <nanogui/screen.h>
#include <GLFW/glfw3.h>
#include "BlueprintCanvas.hpp" // Required for on_modified()

KeyPressCoreNode::KeyPressCoreNode(UUID id)
: DataCoreNode(NodeType::KeyPress, id, nanogui::Color(255, 0, 255, 255))
, mOutput(add_output(PinType::Flow, PinSubType::None))
, mKeyCode(-1)
, mConfigured(false)
, mTriggered(false)
, mWindow(nullptr)
{
}

/**
 * @brief This is now the single source of truth for updating the node's data.
 */
void KeyPressCoreNode::set_data(std::optional<std::variant<Entity, std::string, long, float, bool>> data) { 
    if (data.has_value() && std::holds_alternative<long>(data.value())) {
        mKeyCode = static_cast<int>(std::get<long>(data.value()));
        mConfigured = true;
    } else {
        mKeyCode = -1;
        mConfigured = false;
    }
}

/**
 * @brief This provides the correctly wrapped data for serialization.
 */
std::optional<std::variant<Entity, std::string, long, float, bool>> KeyPressCoreNode::get_data() const {
    if (mConfigured) {
        return static_cast<long>(mKeyCode);
    }
    return std::nullopt;
}

void KeyPressCoreNode::key_event_fired(int action) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        mTriggered = true;
        raise_event(); // This will call evaluate()
        mTriggered = false;
    }
}

bool KeyPressCoreNode::evaluate() {
    // Return true if the event was just triggered, false otherwise.
    // This boolean return value directly controls if the execution flow continues.
    return mTriggered;
}

KeyPressVisualNode::KeyPressVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, KeyPressCoreNode& coreNode)
: VisualBlueprintNode(parent, "Key Press", position, size, coreNode)
, mActionButton(add_data_widget<PassThroughButton>(*this, "Set"))
, mCoreNode(coreNode)
, mListening(false) {
    
    mCoreNode.set_window(screen().glfw_window());
    
    mActionButton.set_callback([this](){
        mActionButton.set_caption("Press Key");
        mListening = true;
    });
}

/**
 * @brief The event handler now uses set_data() to ensure the state persists and
 * also triggers the event flow when the configured key is pressed.
 */
bool KeyPressVisualNode::keyboard_event(int key, int scancode, int action, int modifiers) {
    if (mListening) {
        if (action == GLFW_PRESS) {
            auto* caption = glfwGetKeyName(key, scancode);
            if (caption == nullptr) {
                caption = "Unnamed Key";
            }
            
            mCoreNode.set_data(static_cast<long>(key));
            
            mListening = false;
            mActionButton.set_caption(caption);
            
            mCanvas.on_modified();
        }
        return true;
    }
    
    // If not listening, check if this node should fire an event.
    // This assumes the node receives global keyboard events.
    if (mCoreNode.configured() && key == mCoreNode.keycode()) {
        mCoreNode.key_event_fired(action);
        return true; // Event was handled.
    }
    
    // If not listening and key doesn't match, pass the event to the base class.
    return VisualBlueprintNode::keyboard_event(key, scancode, action, modifiers);
}

/**
 * @brief This now correctly reads the state to set the button caption on load.
 */
void KeyPressVisualNode::perform_layout(NVGcontext *ctx) {
    VisualBlueprintNode::perform_layout(ctx);
    
    if (mCoreNode.configured()) {
        const char* caption = glfwGetKeyName(mCoreNode.keycode(), 0);
        if (caption != nullptr) {
            mActionButton.set_caption(caption);
        } else {
            mActionButton.set_caption("Key: " + std::to_string(mCoreNode.keycode()));
        }
    } else {
        mActionButton.set_caption("Set");
    }
}