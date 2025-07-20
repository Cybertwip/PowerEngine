#include "KeyPressNode.hpp"

#include <nanogui/screen.h>

#include <GLFW/glfw3.h>

KeyPressCoreNode::KeyPressCoreNode(UUID id)
: DataCoreNode(NodeType::KeyPress, id, nanogui::Color(255, 0, 255, 255))
, mOutput(add_output(PinType::Flow, PinSubType::None))
, mKeyCode(-1)
, mConfigured(false)
, mTriggered(false)
{
	set_evaluate([this]() {
		assert(mWindow);
		
		if (configured()) {
			int action = glfwGetKey(mWindow, keycode());
			
			if ((action == GLFW_PRESS || action == GLFW_REPEAT) && !mTriggered) {
				mTriggered = true;
			} else {
				mTriggered = false;
			}
		}
		
		output().can_flow = mTriggered;
	});
	
}

void KeyPressCoreNode::set_data(std::optional<std::variant<Entity, std::string, long, float, bool>> data) {
	if (data.has_value()) {
		mKeyCode = std::get<int>(data.value());
		mConfigured = true;
	} else {
		mKeyCode = -1;
		mConfigured = false;
	}
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

bool KeyPressVisualNode::keyboard_event(int key, int scancode, int action, int modifiers) {
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


void KeyPressVisualNode::perform_layout(NVGcontext *ctx) {
	VisualBlueprintNode::perform_layout(ctx);
	
	auto data = mCoreNode.get_data();
	
	if (data.has_value()) {
		auto* caption = glfwGetKeyName(mCoreNode.keycode(), -1);
		
		if (caption != nullptr) {
			mActionButton.set_caption(caption);
		}
	}
}
