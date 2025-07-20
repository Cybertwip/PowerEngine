#include "KeyReleaseNode.hpp"

#include <nanogui/screen.h>

#include <GLFW/glfw3.h>

KeyReleaseCoreNode::KeyReleaseCoreNode(UUID id)
: DataCoreNode(NodeType::KeyRelease, id, nanogui::Color(255, 0, 255, 255))
, mOutput(add_output(PinType::Flow, PinSubType::None))
, mKeyCode(-1)
, mConfigured(false)
, mTriggered(false) {
	set_evaluate([this]() {
		assert(mWindow);
		if (configured()) {
			int action = glfwGetKey(mWindow, keycode());
			
			output().can_flow = action == GLFW_RELEASE && mTriggered; // Reset the triggered action.
			
			if (action == GLFW_PRESS && !mTriggered) {
				// The key is pressed for the first time.
				mTriggered = true;
				output().can_flow = false; // Triggered action.
			} else if (action == GLFW_RELEASE && mTriggered) {
				// The key is released after being pressed.
				mTriggered = false;
			}
		}
	});
}

void KeyReleaseCoreNode::set_data(std::optional<std::variant<Entity, std::string, long, float, bool>> data) {
	if (data.has_value()) {
		mKeyCode = std::get<int>(data.value());
		mConfigured = true;
	} else {
		mKeyCode = -1;
		mConfigured = false;
	}
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
		auto* caption = glfwGetKeyName(key, scancode);
		
		if (action == GLFW_PRESS && caption != nullptr) {
			mCoreNode.set_keycode(key);
			mListening = false;
			mCoreNode.set_configured(true);
			
			mActionButton.set_caption(caption);
		}
	}
}


void KeyReleaseVisualNode::perform_layout(NVGcontext *ctx) {
	VisualBlueprintNode::perform_layout(ctx);
	
	auto data = mCoreNode.get_data();
	
	if (data.has_value()) {
		auto* caption = glfwGetKeyName(mCoreNode.keycode(), -1);
		
		if (caption != nullptr) {
			mActionButton.set_caption(caption);
		}
	}
}
