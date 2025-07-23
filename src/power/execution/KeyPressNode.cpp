#include "KeyPressNode.hpp"

#include <nanogui/screen.h>
#include <GLFW/glfw3.h>
#include "BlueprintCanvas.hpp" // Required for on_modified()
KeyPressCoreNode::KeyPressCoreNode(UUID id)
: DataCoreNode(NodeType::KeyPress, id, nanogui::Color(255, 0, 255, 255)),
mOutput(add_output(PinType::Flow)), // PinSubType is no longer needed
mKeyCode(-1),
mConfigured(false),
mTriggered(false),
mWindow(nullptr)
{
}

void KeyPressCoreNode::set_data(std::optional<std::any> data) {
	if (data.has_value()) {
		// Safely cast the std::any to a long pointer to check the type.
		if (const long* pval = std::any_cast<long>(&(*data))) {
			mKeyCode = static_cast<int>(*pval);
			mConfigured = true;
		} else {
			mKeyCode = -1;
			mConfigured = false;
		}
	} else {
		mKeyCode = -1;
		mConfigured = false;
	}
}

std::optional<std::any> KeyPressCoreNode::get_data() const {
	if (mConfigured) {
		// Wrap the keycode in std::any for serialization.
		return static_cast<long>(mKeyCode);
	}
	return std::nullopt;
}

void KeyPressCoreNode::key_event_fired(int action) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		mTriggered = true;
		raise_event(); // This will call evaluate() and propagate the flow.
		mTriggered = false;
	}
}

bool KeyPressCoreNode::evaluate() {
	// Return true if the event was just triggered, false otherwise.
	// This boolean return value directly controls if the execution flow continues.
	return mTriggered;
}

KeyPressVisualNode::KeyPressVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, KeyPressCoreNode& coreNode)
: VisualBlueprintNode(parent, "Key Press", position, size, coreNode),
mActionButton(add_data_widget<PassThroughButton>(*this, "Set")),
mCoreNode(coreNode),
mListening(false)
{
	mCoreNode.set_window(screen().glfw_window());
	
	mActionButton.set_callback([this](){
		mActionButton.set_caption("Press Key...");
		mListening = true;
	});
}

bool KeyPressVisualNode::keyboard_event(int key, int scancode, int action, int modifiers) {
	if (mListening) {
		if (action == GLFW_PRESS) {
			// Update the core node's data with the new keycode.
			mCoreNode.set_data(static_cast<long>(key));
			
			mListening = false;
			
			// Update the button caption to reflect the new key.
			const char* caption = glfwGetKeyName(key, scancode);
			mActionButton.set_caption(caption ? caption : "Unnamed");
			
			mCanvas.on_modified();
		}
		return true; // We handled the event.
	}
	
	// If not listening, check if this node should fire its event.
	if (mCoreNode.configured() && key == mCoreNode.keycode()) {
		mCoreNode.key_event_fired(action);
		return true; // Event was handled.
	}
	
	// If not listening and key doesn't match, pass the event to the base class.
	return VisualBlueprintNode::keyboard_event(key, scancode, action, modifiers);
}

void KeyPressVisualNode::perform_layout(NVGcontext *ctx) {
	VisualBlueprintNode::perform_layout(ctx);
	
	// This ensures the button caption is correct when the node is first created or loaded.
	if (!mListening && mCoreNode.configured()) {
		const char* caption = glfwGetKeyName(mCoreNode.keycode(), 0);
		if (caption != nullptr) {
			mActionButton.set_caption(caption);
		} else {
			mActionButton.set_caption("Key: " + std::to_string(mCoreNode.keycode()));
		}
	} else if (!mListening) {
		mActionButton.set_caption("Set");
	}
}
