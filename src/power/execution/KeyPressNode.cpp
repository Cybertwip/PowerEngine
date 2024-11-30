#include "KeyPressNode.hpp"

#include <nanogui/screen.h>

#include <GLFW/glfw3.h>

KeyPressNode::KeyPressNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, long long id, nanogui::Vector2i size, std::function<int()> id_registrator_lambda)
: BlueprintDataNode(parent, NodeType::KeyPress, "Key Press", size, id, nanogui::Color(255, 0, 255, 255)), mKeyCode(-1), mListening(false), mConfigured(false), mTriggered(false), mActionButton(add_data_widget<PassThroughButton>(*this, "Set")) {
	auto& output_flow = add_output(id_registrator_lambda(), this->id, "", PinType::Flow, PinSubType::None);
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


void KeyPressNode::set_data(std::optional<std::variant<Entity, std::string, int, float, bool>> data) {
	if (data.has_value()) {
		mKeyCode = std::get<int>(data.value());
		auto* caption = glfwGetKeyName(mKeyCode, -1);
		
		if (caption != nullptr) {
			mActionButton.set_caption(caption);
		}
		
		mConfigured = true;
		
	} else {
		mKeyCode = -1;
		mConfigured = false;
	}
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

