#include "KeyReleaseNode.hpp" // Assumes the header is named correctly

#include <nanogui/screen.h>
#include <GLFW/glfw3.h>
#include "BlueprintCanvas.hpp" // Required for on_modified()

KeyReleaseCoreNode::KeyReleaseCoreNode(UUID id)
: DataCoreNode(NodeType::KeyRelease, id, nanogui::Color(255, 100, 255, 255))
, mOutput(add_output(PinType::Flow, PinSubType::None))
, mKeyCode(-1)
, mConfigured(false)
, mTriggered(false)
, mWindow(nullptr)
{
    // The constructor is now empty.
    // All logic is handled by the virtual evaluate() and event-driven methods.
}

void KeyReleaseCoreNode::set_data(std::optional<std::variant<Entity, std::string, long, float, bool>> data) {   
    if (data.has_value() && std::holds_alternative<long>(data.value())) {
        mKeyCode = static_cast<int>(std::get<long>(data.value()));
        mConfigured = true;
    } else {
        mKeyCode = -1;
        mConfigured = false;
    }
}

std::optional<std::variant<Entity, std::string, long, float, bool>> KeyReleaseCoreNode::get_data() const {
    if (mConfigured) {
        return static_cast<long>(mKeyCode);
    }
    return std::nullopt;
}

void KeyReleaseCoreNode::key_event_fired(int action) {
    // We only care about the key being released.
    if (action == GLFW_RELEASE) {
        mTriggered = true;
        raise_event(); // Triggers the call to this node's evaluate()
        mTriggered = false;
    }
}

bool KeyReleaseCoreNode::evaluate() {
    // This function is called by raise_event().
    // It returns true only if the event was a key release, allowing the flow to continue.
    return mTriggered;
}

KeyReleaseVisualNode::KeyReleaseVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, KeyReleaseCoreNode& coreNode)
: VisualBlueprintNode(parent, "Key Release", position, size, coreNode)
, mActionButton(add_data_widget<PassThroughButton>(*this, "Set"))
, mCoreNode(coreNode)
, mListening(false) {
    
    mCoreNode.set_window(screen().glfw_window());
    
    mActionButton.set_callback([this](){
        mActionButton.set_caption("Press Key");
        mListening = true;
    });
}

bool KeyReleaseVisualNode::keyboard_event(int key, int scancode, int action, int modifiers) {
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
    if (mCoreNode.configured() && key == mCoreNode.keycode()) {
        mCoreNode.key_event_fired(action);
        return true; // Event was handled.
    }
    
    // Pass the event to the base class if we didn't handle it.
    return VisualBlueprintNode::keyboard_event(key, scancode, action, modifiers);
}

void KeyReleaseVisualNode::perform_layout(NVGcontext *ctx) {
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