#include "KeyReleaseNode.hpp"

namespace blueprint {
KeyReleaseNode::KeyReleaseNode(BlueprintCanvas& parent, const std::string& title, nanogui::Vector2i size, int id, int flow_pin_id)
: BlueprintNode(parent, title, size, id, nanogui::Color(255, 0, 255, 255)), mKeyCode(-1), mListening(false), mConfigured(false), mTriggered(false),
mActionButton(add_data_widget<nanogui::Button>("Setup")) {
	auto& output_flow = add_output(flow_pin_id, this->id, "", PinType::Flow);
	root_node = true;
	evaluate = [this, &output_flow]() {
		output_flow.can_flow = mTriggered;
		mTriggered = false;
	};
	
	mActionButton.set_fixed_size(nanogui::Vector2i(32, 10));
	
	mActionButton.set_callback([this](){
		mActionButton.set_caption("Press Key");
		mListening = true;
		request_focus();
	});
}

bool KeyReleaseNode::keyboard_event(int key, int scancode, int action, int modifiers) {
	if (mListening) {
		auto* caption = glfwGetKeyName(key, scancode);

		if (action == GLFW_RELEASE && caption != nullptr) {
			mKeyCode = key;
			mListening = false;
			mConfigured = true;
			mActionButton.set_caption(caption);
		}
	} else {
		if (mConfigured) {
			if (mKeyCode == key && action == GLFW_RELEASE) {
				mTriggered = true;
			}
		}
	}
}

}