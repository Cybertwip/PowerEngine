#include "KeyReleaseNode.hpp"

#include <nanogui/screen.h>
#include <GLFW/glfw3.h>
#include "BlueprintCanvas.hpp" // Required for on_modified()

KeyReleaseCoreNode::KeyReleaseCoreNode(UUID id)
: DataCoreNode(NodeType::KeyRelease, id, nanogui::Color(255, 100, 255, 255))
, mOutput(add_output(PinType::Flow, PinSubType::None))
, mKeyCode(-1)
, mConfigured(false)
, mWasPressed(false)
, mWindow(nullptr)
{
	// FIX: The evaluation logic is now correct and stateful.
	set_evaluate([this]() {
		assert(mWindow);
		bool trigger_now = false;
		
		if (configured()) {
			int action = glfwGetKey(mWindow, keycode());
			
			if (action == GLFW_PRESS || action == GLFW_REPEAT) {
				mWasPressed = true;
			} else if (action == GLFW_RELEASE) {
				if (mWasPressed) {
					// The key was down and is now up, so trigger the release event.
					trigger_now = true;
				}
				// The key is now up, so reset the pressed state.
				mWasPressed = false;
			}
		} else {
			mWasPressed = false; // Reset if not configured.
		}
		
		output().can_flow = trigger_now;
	});
}

/**
 * @brief This is now the single source of truth for updating the node's data.
 */
void KeyReleaseCoreNode::set_data(std::optional<std::variant<Entity, std::string, long, float, bool>> data) {	
	if (data.has_value() && std::holds_alternative<long>(data.value())) {
		// FIX: Update the specific state of this node from the data.
		mKeyCode = static_cast<int>(std::get<long>(data.value()));
		mConfigured = true;
	} else {
		// FIX: Reset to the default state if data is null or the wrong type.
		mKeyCode = -1;
		mConfigured = false;
	}
}

/**
 * @brief This provides the correctly wrapped data for serialization.
 */
std::optional<std::variant<Entity, std::string, long, float, bool>> KeyReleaseCoreNode::get_data() const {
	if (mConfigured) {
		// FIX: Return the keycode as a 'long' inside the variant and optional.
		return static_cast<long>(mKeyCode);
	}
	return std::nullopt;
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

/**
 * @brief The event handler now uses set_data() to ensure the state persists.
 */
bool KeyReleaseVisualNode::keyboard_event(int key, int scancode, int action, int modifiers) {
	if (mListening) {
		if (action == GLFW_PRESS) {
			auto* caption = glfwGetKeyName(key, scancode);
			if (caption == nullptr) {
				caption = "Unnamed Key";
			}
			
			// FIX: Use set_data to update the core node. This ensures the data
			// is stored correctly for serialization.
			mCoreNode.set_data(static_cast<long>(key));
			
			mListening = false;
			mActionButton.set_caption(caption);
			
			// FIX: Notify the canvas that a change has occurred.
			mCanvas.on_modified();
		}
		// FIX: Return true because the event was handled.
		return true;
	}
	
	// If not listening, pass the event to the base class.
	return VisualBlueprintNode::keyboard_event(key, scancode, action, modifiers);
}

/**
 * @brief This now correctly reads the state to set the button caption on load.
 */
void KeyReleaseVisualNode::perform_layout(NVGcontext *ctx) {
	VisualBlueprintNode::perform_layout(ctx);
	
	if (mCoreNode.configured()) {
		// Use 0 for scancode to get the primary name for the key.
		const char* caption = glfwGetKeyName(mCoreNode.keycode(), 0);
		if (caption != nullptr) {
			mActionButton.set_caption(caption);
		} else {
			// Fallback for keys that might not have a name.
			mActionButton.set_caption("Key: " + std::to_string(mCoreNode.keycode()));
		}
	} else {
		mActionButton.set_caption("Set");
	}
}
