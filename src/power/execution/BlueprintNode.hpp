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
#include <variant>

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

enum class PinSubType {
	None,
	Actor,
	Light,
	Camera,
	Animation,
	Sequence,
	Composition
};

enum class PinKind {
	Output,
	Input
};

class BlueprintNode;
class CoreNode;
class Link;

struct Entity {
	int id;
	std::optional<std::variant<std::string, int, float, bool>> payload;
};

class BlueprintCanvas;

class PassThroughWidget : public nanogui::Widget {
public:
	PassThroughWidget(nanogui::Window& parent)
	: nanogui::Widget(parent), mWindow(parent) {
		
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


class PassThroughButton : public nanogui::Button {
public:
	PassThroughButton(nanogui::Widget& parent, nanogui::Window& window, const std::string &caption = "", int icon = 0)
	: nanogui::Button(parent, caption, icon), mWindow(window) {
		
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

class CorePin {
public:
	UUID id;
	CoreNode& node;
	PinType type;
	PinSubType subtype;
	PinKind kind;
	bool can_flow = false;
	
	std::vector<Link*> links;
	
	CorePin(CoreNode& parent, UUID id, PinType type, PinSubType subtype = PinSubType::None)
	: id(id), node(parent), type(type), subtype(subtype), kind(PinKind::Input) {
		if (type == PinType::Bool) {
			set_data(true);
		} else if (type == PinType::String) {
			set_data("");
		} else if (type == PinType::Float) {
			set_data(0.0f);
		} else if (type == PinType::Flow) {
			set_data(true);
		}
	}
	
	virtual std::optional<std::variant<Entity, std::string, int, float, bool>> get_data() {
		return mData;
	}
	
	virtual void set_data(std::optional<std::variant<Entity, std::string, long, float, bool>> data) {
		mData = data;
	}
	
private:
	std::optional<std::variant<Entity, std::string, long, float, bool>> mData;
};

class CoreNode {
public:
	NodeType type;
	UUID id;
	nanogui::Color color;
	bool root_node = false;
	nanogui::Vector2i position;
	
	void link() {
		if (mLink) {
			mLink();
		}
	}
	
	bool evaluate() {
		if (mEvaluate) {
			mEvaluate();
			return true;
		} else {
			return false;
		}
	}
	
	void set_link(std::function<void()> link) {
		mLink = link;
	}
	
	void set_evaluate(std::function<void()> evaluate) {
		mEvaluate = evaluate;
	}

	CoreNode(NodeType type, UUID id, nanogui::Color color = nanogui::Color(255, 255, 255, 255)) : type(type), id(id), color(color) {
		
	}
	
	virtual ~CoreNode() = default;
	
	CorePin& add_input(PinType pin_type, PinSubType pin_subtype) {
				
		auto input = std::make_unique<CorePin>(*this, get_next_id(), pin_type, pin_subtype);
		
		inputs.push_back(std::move(input));
		
		return *inputs.back();
	}
	
	
	CorePin& add_output(PinType pin_type, PinSubType pin_subtype) {
		
		auto output = std::make_unique<CorePin>(*this, get_next_id(), pin_type, pin_subtype);
		
		outputs.push_back(std::move(output));
		
		return *outputs.back();
	}
	
	void build();
	
	void reset_flow();
	
	const std::vector<std::unique_ptr<CorePin>>& get_inputs() const {
		return inputs;
	}
	
	const std::vector<std::unique_ptr<CorePin>>& get_outputs() const {
		return outputs;
	}
	
	void set_position(const nanogui::Vector2i& position) {
		this->position = position;
	}
	
	CorePin& get_pin(UUID id) {
		// Search through input pins
		for (const auto& input : inputs) {
			if (input->id == id) {
				return *input.get();
			}
		}
		
		// Search through output pins
		for (const auto& output : outputs) {
			if (output->id == id) {
				return *output.get();
			}
		}
		
		// Pin not found
		assert(false);
	}

		
private:
	UUID get_next_id();
	
	std::function<void()> mLink;
	std::function<void()> mEvaluate;
	
	std::vector<std::unique_ptr<CorePin>> inputs;
	std::vector<std::unique_ptr<CorePin>> outputs;
};

class DataCoreNode : public CoreNode {
public:
	DataCoreNode(NodeType type, UUID id, nanogui::Color color = nanogui::Color(255, 255, 255, 255)) : CoreNode(type, id, color) {
		
	}
	virtual std::optional<std::variant<Entity, std::string, long, float, bool>> get_data() const = 0;
	
	virtual void set_data(std::optional<std::variant<Entity, std::string, long, float, bool>> data) = 0;
};


class VisualLink;
class VisualBlueprintNode;

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
	}
	
	std::function<void()> mHoverCallback;
	std::function<void()> mClickCallback;

	CorePin& mCorePin;
};

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
		// Search through visual pins
		for (const auto& pin : mVisualPins) {
			if (pin->core_pin().id == id) {
				return pin.get();
			}
		}

		// Pin not found
		return nullptr;
	}

	
protected:
	virtual void perform_layout(NVGcontext *ctx) override;

	std::vector<std::unique_ptr<nanogui::Widget>> data_widgets;
	
private:
	void create_visual_pins();
	void draw(NVGcontext *ctx) override;
	BlueprintCanvas& mCanvas;
	CoreNode& mCoreNode;
	std::unique_ptr<nanogui::Widget> mFlowContainer;
	std::unique_ptr<nanogui::Widget> mColumnContainer;
	std::unique_ptr<nanogui::Widget> mLeftColumn;
	std::unique_ptr<nanogui::Widget> mDataColumn;
	std::unique_ptr<nanogui::Widget> mRightColumn;
	std::vector<std::unique_ptr<VisualPin>> mVisualPins;
};

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
	
	int get_id() const {
		return id;
	}

private:
	int id;
	CorePin& mStart;
	CorePin& mEnd;
	nanogui::Color color;
};

class VisualLink {
public:
	VisualLink(VisualPin& start, VisualPin& end)
	: mStart(start)
	, mEnd(end) {
		
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
