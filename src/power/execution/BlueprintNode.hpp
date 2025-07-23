#pragma once

#include "execution/BlueprintCanvas.hpp"
#include "serialization/UUID.hpp"

#include <nanogui/icons.h>
#include <nanogui/toolbutton.h>
#include <nanogui/vector.h>
#include <nanogui/textbox.h>
#include <nanogui/window.h>

#include <GLFW/glfw3.h>

#include <functional>
#include <optional>
#include <string>
#include <any> // Changed from <variant>
#include <vector>
#include <memory>
#include <cassert>

// Forward declarations
class BlueprintCanvas;
class BlueprintNode;
class CoreNode;
class Link;
class VisualPin;
class VisualBlueprintNode;


enum class NodeType {
	KeyPress,
	KeyRelease,
	String,
	Print
};

enum class PinType {
	Flow,
	Bool,
	Int,
	Float,
	String,
	Object,
	Function,
	Delegate,
};

// Removed PinSubType enum as it is no longer needed with std::any

enum class PinKind {
	Output,
	Input
};

struct Entity {
	UUID id;
	// Payload is now std::any for greater flexibility
	std::optional<std::any> payload;
};

// This widget passes mouse and keyboard events to its parent window.
// Useful for creating custom node UI that doesn't block window dragging.
class PassThroughWidget : public nanogui::Widget {
public:
	PassThroughWidget(nanogui::Window& parent)
	: nanogui::Widget(&parent), mWindow(parent) {
	}
	
private:
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down,
							int modifiers) override {
		mWindow.mouse_button_event(p, button, down, modifiers);
		return nanogui::Widget::mouse_button_event(p, button, down, modifiers);
	}
	
	bool mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {
		mWindow.mouse_drag_event(p, rel, button, modifiers);
		return nanogui::Widget::mouse_drag_event(p, rel, button, modifiers);
	}
	
	bool keyboard_event(int key, int scancode, int action, int modifiers) override {
		return mWindow.keyboard_event(key, scancode, action, modifiers);
	}
	
	nanogui::Window& mWindow;
};

// This button passes events to its parent window, similar to PassThroughWidget.
class PassThroughButton : public nanogui::Button {
public:
	PassThroughButton(nanogui::Widget& parent, nanogui::Window& window, const std::string &caption = "", int icon = 0)
	: nanogui::Button(&parent, caption, icon), mWindow(window) {
	}
	
private:
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down,
							int modifiers) override {
		mWindow.mouse_button_event(p, button, down, modifiers);
		return nanogui::Button::mouse_button_event(p, button, down, modifiers);
	}
	
	bool mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {
		mWindow.mouse_drag_event(p, rel, button, modifiers);
		return nanogui::Button::mouse_drag_event(p, rel, button, modifiers);
	}
	
	bool keyboard_event(int key, int scancode, int action, int modifiers) override {
		return mWindow.keyboard_event(key, scancode, action, modifiers);
	}
	
	nanogui::Window& mWindow;
};

// Represents the logical connection point on a node.
class CorePin {
public:
	UUID id;
	CoreNode& node;
	PinType type;
	PinKind kind;
	
	std::vector<Link*> links;
	
	// Constructor updated to remove PinSubType
	CorePin(CoreNode& parent, UUID id, PinType type)
	: id(id), node(parent), type(type), kind(PinKind::Input) {
		// Initialize with default data based on type
		if (type == PinType::Bool) {
			set_data(true);
		} else if (type == PinType::String) {
			set_data(std::string("")); // Use std::string for clarity with std::any
		} else if (type == PinType::Float) {
			set_data(0.0f);
		} else if (type == PinType::Flow) {
			// Flow pins can use a bool to indicate if they are "active"
			set_data(true);
		}
	}
	
	// get_data now returns an optional std::any
	virtual std::optional<std::any> get_data() {
		return mData;
	}
	
	// set_data now accepts an optional std::any
	virtual void set_data(std::optional<std::any> data) {
		mData = data;
	}
	
private:
	// The underlying data is now stored in an optional std::any
	std::optional<std::any> mData;
};

// Represents the logical link between two CorePins.
class Link {
public:
	Link(UUID id, CorePin& start, CorePin& end)
	: id(id), mStart(start), mEnd(end), color(nanogui::Color(255, 255, 255, 255)) {
	}
	CorePin& get_start() const {
		return mStart;
	}
	CorePin& get_end() const {
		return mEnd;
	}
	UUID get_id() const {
		return id;
	}
private:
	UUID id;
	CorePin& mStart;
	CorePin& mEnd;
	nanogui::Color color;
};

// Represents the visual link between two VisualPins.
class VisualLink {
public:
	VisualLink(VisualPin& start, VisualPin& end)
	: mStart(start), mEnd(end) {
	}
	VisualPin& get_start() const {
		return mStart;
	}
	VisualPin& get_end() const {
		return mEnd;
	}
private:
	VisualPin& mStart;
	VisualPin& mEnd;
};

// Represents the logical core of a blueprint node.
class CoreNode {
public:
	NodeType type;
	UUID id;
	nanogui::Color color;
	bool root_node = false;
	nanogui::Vector2i position;
	
	/**
	 * @brief Executes the node's internal logic.
	 * @return True to continue the execution flow, false to stop it.
	 * Derived classes must override this to implement their behavior.
	 */
	virtual bool evaluate() {
		return false;
	}
	
	void raise_event();
	
	CoreNode(NodeType type, UUID id, nanogui::Color color = nanogui::Color(255, 255, 255, 255)) : type(type), id(id), color(color) {
	}
	
	virtual ~CoreNode() = default;
	
	// add_input updated to remove PinSubType
	CorePin& add_input(PinType pin_type) {
		auto input = std::make_unique<CorePin>(*this, get_next_id(), pin_type);
		inputs.push_back(std::move(input));
		return *inputs.back();
	}
	
	// add_output updated to remove PinSubType
	CorePin& add_output(PinType pin_type) {
		auto output = std::make_unique<CorePin>(*this, get_next_id(), pin_type);
		outputs.push_back(std::move(output));
		return *outputs.back();
	}
	
	void build();
	
	const std::vector<std::unique_ptr<CorePin>>& get_inputs() const {
		return inputs;
	}
	
	const std::vector<std::unique_ptr<CorePin>>& get_outputs() const {
		return outputs;
	}
	
	void set_position(const nanogui::Vector2i& pos) {
		this->position = pos;
	}
	
	CorePin& get_pin(UUID pin_id) {
		// Search through input pins
		for (const auto& input : inputs) {
			if (input->id == pin_id) {
				return *input.get();
			}
		}
		
		// Search through output pins
		for (const auto& output : outputs) {
			if (output->id == pin_id) {
				return *output.get();
			}
		}
		
		// Pin not found, this should not happen in a valid state
		assert(false && "Pin not found");
		// Return a reference to avoid compiler warnings, but the assert should have fired.
		// In a real application, you might throw an exception here.
		return *inputs.front();
	}
	
	
private:
	UUID get_next_id();
	std::vector<std::unique_ptr<CorePin>> inputs;
	std::vector<std::unique_ptr<CorePin>> outputs;
};

// A specialization of CoreNode for nodes that primarily handle data.
class DataCoreNode : public CoreNode {
public:
	DataCoreNode(NodeType type, UUID id, nanogui::Color color = nanogui::Color(255, 255, 255, 255)) : CoreNode(type, id, color) {
	}
	
	// get_data now returns an optional std::any
	virtual std::optional<std::any> get_data() const = 0;
	
	// set_data now accepts an optional std::any
	virtual void set_data(std::optional<std::any> data) = 0;
};


// Represents the visual, clickable pin on a blueprint node.
class VisualPin : public nanogui::ToolButton {
public:
	VisualPin(nanogui::Widget& parent, CorePin& core_pin);
	
	CoreNode& node() {
		return mCorePin.node;
	}
	
	PinType get_type() {
		return mCorePin.type;
	}
	
	CorePin& core_pin() {
		return mCorePin;
	}
	
	void set_hover_callback(std::function<void()> callback) {
		mHoverCallback = callback;
	}
	
	void set_click_callback(std::function<void()> callback) {
		mClickCallback = callback;
	}
	
private:
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {
		nanogui::Widget::mouse_button_event(p, button, down, modifiers);
		if (button == GLFW_MOUSE_BUTTON_1 && !down) {
			if (mHoverCallback) {
				mHoverCallback();
			}
		} else if (button == GLFW_MOUSE_BUTTON_1) {
			if (mClickCallback) {
				mClickCallback();
			}
		}
		// This event is not returned to allow the parent window to process it for dragging.
		return false;
	}
	
	std::function<void()> mHoverCallback;
	std::function<void()> mClickCallback;
	
	CorePin& mCorePin;
};

// Represents the visual window for a blueprint node.
class VisualBlueprintNode : public nanogui::Window {
public:
	VisualBlueprintNode(BlueprintCanvas& parent, const std::string& name, nanogui::Vector2i position, nanogui::Vector2i size, CoreNode& coreNode);
	
	template<typename T, typename... Args>
	T& add_data_widget(Args&&... args){
		auto data_widget = std::make_unique<T>(*mDataColumn, std::forward<Args>(args)...);
		data_widget->set_fixed_size(nanogui::Vector2i(fixed_size().x() - 48, 22));
		auto& data_widget_ref = *data_widget;
		data_widgets.push_back(std::move(data_widget));
		return data_widget_ref;
	}
	
	CoreNode& core_node() {
		return mCoreNode;
	}
	
	VisualPin* find_pin(UUID id) {
		for (const auto& pin : mVisualPins) {
			if (pin->core_pin().id == id) {
				return pin.get();
			}
		}
		return nullptr; // Pin not found
	}
	
	
protected:
	void perform_layout(NVGcontext *ctx) override;
	
	std::vector<std::unique_ptr<nanogui::Widget>> data_widgets;
	BlueprintCanvas& mCanvas;
	
private:
	void create_visual_pins();
	void draw(NVGcontext *ctx) override;
	CoreNode& mCoreNode;
	std::unique_ptr<nanogui::Widget> mFlowContainer;
	std::unique_ptr<nanogui::Widget> mColumnContainer;
	std::unique_ptr<nanogui::Widget> mLeftColumn;
	std::unique_ptr<nanogui::Widget> mDataColumn;
	std::unique_ptr<nanogui::Widget> mRightColumn;
	std::vector<std::unique_ptr<VisualPin>> mVisualPins;
};
