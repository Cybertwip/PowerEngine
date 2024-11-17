#include "KeyPressNode.hpp"

#include <nanogui/screen.h>

#include <GLFW/glfw3.h>

namespace blueprint {
KeyPressNode::KeyPressNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, const std::string& title, nanogui::Vector2i size, int id, int flow_pin_id)
: BlueprintNode(parent, NodeType::KeyPress, title, size, id, nanogui::Color(255, 0, 255, 255)), mKeyCode(-1), mListening(false), mConfigured(false), mTriggered(false), mActionButton(add_data_widget<PassThroughButton>(*this, "Set")) {
	auto& output_flow = add_output(flow_pin_id, this->id, "", PinType::Flow);
	root_node = true;
	evaluate = [this, &output_flow]() {
		if (mConfigured) {
			int action = glfwGetKey(screen().glfw_window(), mKeyCode);
		
			if ((action == GLFW_PRESS || action == GLFW_REPEAT) && !mTriggered) {
				mTriggered = true;
			} else {
				mTriggered = false;
			}
		}

		output_flow.can_flow = mTriggered;
	};
		
	mActionButton.set_callback([this](){
		mActionButton.set_caption("Press Key");
		mListening = true;
	});
}

bool KeyPressNode::keyboard_event(int key, int scancode, int action, int modifiers) {
	if (mListening) {
		auto* caption = glfwGetKeyName(key, scancode);

		if (action == GLFW_PRESS && caption != nullptr) {
			mKeyCode = key;
			mListening = false;
			mConfigured = true;
			
			mActionButton.set_caption(caption);
		}
	}
}

}
