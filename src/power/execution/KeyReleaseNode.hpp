#pragma once

#include "BlueprintNode.hpp"

#include <nanogui/button.h>

#include <memory>
#include <optional>
#include <string>
#include <variant>

class BlueprintCanvas;

class KeyReleaseCoreNode : public DataCoreNode {
public:
	KeyReleaseCoreNode(UUID id);
	
	std::optional<std::variant<Entity, std::string, long, float, bool>> get_data() const override {
		return mKeyCode;
	}
	
	void set_data(std::optional<std::variant<Entity, std::string, long, float, bool>> data) override;
	
	int keycode() {
		return mKeyCode;
	}
	
	bool configured() {
		return mConfigured;
	}
	
	void set_keycode(int keycode) {
		mKeyCode = keycode;
	}
	
	void set_configured(bool configured) {
		mConfigured = configured;
	}
	
	CorePin& output() {
		return mOutput;
	}
	
	void set_window(GLFWwindow* window) {
		mWindow = window;
	}
	
private:
	int mKeyCode;
	bool mConfigured;
	bool mTriggered;
	
	CorePin& mOutput;
	GLFWwindow* mWindow;
};

class KeyReleaseVisualNode : public VisualBlueprintNode {
public:
	KeyReleaseVisualNode(BlueprintCanvas& parent, nanogui::Vector2i position, nanogui::Vector2i size, KeyReleaseCoreNode& coreNode);
	
	bool keyboard_event(int key, int scancode, int action, int modifiers) override;
	
	void perform_layout(NVGcontext *ctx) override;
	
private:
	nanogui::Button& mActionButton;
	KeyReleaseCoreNode& mCoreNode;
	
	bool mTriggered;
	bool mListening;
};

