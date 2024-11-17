#include "KeyPressNode.hpp"

namespace blueprint {
KeyPressNode::KeyPressNode(BlueprintCanvas& parent, const std::string& title, nanogui::Vector2i size, int id, int flow_pin_id)
: BlueprintNode(parent, title, size, id, nanogui::Color(255, 0, 255, 255)), mKeyCode(-1), mListening(false), mConfigured(false), mTriggered(false) {
	auto& output_flow = add_output(flow_pin_id, this->id, "", PinType::Flow);
	root_node = true;
	evaluate = [this, &output_flow]() {
		output_flow.can_flow = mTriggered;
		
		mTriggered = false;
	};
	
	mButton = std::make_unique<nanogui::Button>(*this, "Setup");

	mButton->set_fixed_size(nanogui::Vector2i(32, 10));
	
	mButton->set_callback([this](){
		mListening = true;
	});
}

bool KeyPressNode::keyboard_event(int key, int scancode, int action, int modifiers) {
	if (mListening) {
		if (action == GLFW_PRESS) {
			mKeyCode = key;
			mListening = false;
			mConfigured = true;
			mButton->set_caption(glfwGetKeyName(key, scancode));
		}
	} else {
		if (mConfigured) {
			if (mKeyCode == key && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
				mTriggered = true;
			}
		}
	}
}

}
