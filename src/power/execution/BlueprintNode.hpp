#pragma once

#include "execution/BlueprintCanvas.hpp"

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

class Pin : public nanogui::ToolButton {
public:
	int id;
	int node_id;
	BlueprintNode* node;
	PinType type;
	PinSubType subtype;
	PinKind kind;
	bool can_flow = false;
	
	std::vector<Link*> links;
	
	Pin(nanogui::Widget& parent, int id, int node_id, const std::string& label, PinType type, PinSubType subtype = PinSubType::None)
	: nanogui::ToolButton(parent, FA_CIRCLE_NOTCH, label), id(id), node_id(node_id), node(nullptr), type(type), subtype(subtype), kind(PinKind::Input) {
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
	
	virtual void set_data(std::optional<std::variant<Entity, std::string, int, float, bool>> data) {
		mData = data;
	}
	
	void set_hover_callback(std::function<void()> callback) {
		mHoverCallback = callback;
	}

	void set_click_callback(std::function<void()> callback) {
		mClickCallback = callback;
	}

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
	
private:
	std::optional<std::variant<Entity, std::string, int, float, bool>> mData;
	
	std::function<void()> mHoverCallback;
	std::function<void()> mClickCallback;
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

class BlueprintNode : public nanogui::Window {
public:
	NodeType type;
	long long id;
	nanogui::Color color;
	bool root_node = false;
	std::function<void()> link;
	std::function<void()> evaluate;
	
	BlueprintNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, NodeType type, const std::string& name, nanogui::Vector2i size, long long id, nanogui::Color color = nanogui::Color(255, 255, 255, 255));
	
	template<typename T, typename... Args>
	T& add_data_widget(Args&&... args){
		auto data_widget = std::make_unique<T>(*mDataColumn, std::forward<Args>(args)...);
		
		data_widget->set_fixed_size(nanogui::Vector2i(fixed_size().x() - 48, 22));
		
		auto& data_widget_ref = *data_widget;
		
		data_widgets.push_back(std::move(data_widget));
		
		return data_widget_ref;
	}
	
	
	template<typename T = Pin, typename... Args>
	Pin& add_input(int node_id, const std::string& label, PinType pin_type, PinSubType pin_subtype, Args &&...args) {
		
		auto& parent = pin_type == PinType::Flow ? *mFlowContainer : *mLeftColumn;
		
		auto input = std::make_unique<T>(parent, get_next_id(), node_id, label, pin_type, pin_subtype, std::forward<Args>(args)...);

		input->set_programmable(true);
		
		if (pin_type == PinType::Flow) {
			input->set_icon(FA_PLAY);
		}
		
		if (mCanvas.has_value()) {
			auto& input_ref = *input;
			
			input->set_hover_callback([this, &input_ref](){
//				mCanvas->get().on_input_pin_clicked(input_ref);
			});
		}
		
		input->set_fixed_size(nanogui::Vector2i(22, 22));
		
		if (pin_type == PinType::Flow) {
			input->set_position(nanogui::Vector2i(5, 3));
		}
		
		inputs.push_back(std::move(input));
		
		return *inputs.back();
	}

	template<typename T = Pin, typename... Args>
	Pin& add_output(int node_id, const std::string& label, PinType pin_type, PinSubType pin_subtype, Args &&...args) {
		auto& parent = pin_type == PinType::Flow ? *mFlowContainer : *mRightColumn;
		
		auto output = std::make_unique<T>(parent, get_next_id(), node_id, label, pin_type, pin_subtype, std::forward<Args>(args)...);
		
		output->set_programmable(true);
		
		if (pin_type == PinType::Flow) {
			output->set_icon(FA_PLAY);
		}
		
		if (mCanvas.has_value()) {
			auto& output_ref = *output;
			
			output->set_click_callback([this, &output_ref](){
				//mCanvas->get().on_output_pin_clicked(output_ref);
			});
			
		}
		
		output->set_fixed_size(nanogui::Vector2i(22, 22));
		
		if (pin_type == PinType::Flow) {
			output->set_position(nanogui::Vector2i(fixed_size().x() - 22 - 5, 3));
		}
		
		outputs.push_back(std::move(output));
		
		return *outputs.back();
	}
	void build();
	
	void reset_flow();
	
	const std::vector<std::unique_ptr<Pin>>& get_inputs() {
		return inputs;
	}

	const std::vector<std::unique_ptr<Pin>>& get_outputs() {
		return outputs;
	}

protected:
	std::vector<std::unique_ptr<Pin>> inputs;
	std::vector<std::unique_ptr<nanogui::Widget>> data_widgets;
	std::vector<std::unique_ptr<Pin>> outputs;
	
private:
	long long get_next_id();

	void perform_layout(NVGcontext *ctx) override;
	void draw(NVGcontext *ctx) override;
	std::optional<std::reference_wrapper<BlueprintCanvas>> mCanvas;
	std::unique_ptr<nanogui::Widget> mFlowContainer;
	std::unique_ptr<nanogui::Widget> mColumnContainer;
	std::unique_ptr<nanogui::Widget> mLeftColumn;
	std::unique_ptr<nanogui::Widget> mDataColumn;
	std::unique_ptr<nanogui::Widget> mRightColumn;
	
	long long next_id;
};

class CorePin {
public:
	int id;
	int node_id;
	CoreNode* node;
	PinType type;
	PinSubType subtype;
	PinKind kind;
	bool can_flow = false;
	
	std::vector<Link*> links;
	
	CorePin(int id, int node_id, PinType type, PinSubType subtype = PinSubType::None)
	: id(id), node_id(node_id), node(nullptr), type(type), subtype(subtype), kind(PinKind::Input) {
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
	
	virtual void set_data(std::optional<std::variant<Entity, std::string, int, float, bool>> data) {
		mData = data;
	}
	
private:
	std::optional<std::variant<Entity, std::string, int, float, bool>> mData;
};

class CoreNode {
public:
	NodeType type;
	long long id;
	nanogui::Color color;
	bool root_node = false;
	nanogui::Vector2i position;
	
	std::function<void()> link;
	std::function<void()> evaluate;
	
	CoreNode(NodeType type, long long id, nanogui::Color color = nanogui::Color(255, 255, 255, 255)) : type(type), id(id), color(color), next_id(1) {
		
	}
	
	CorePin& add_input(PinType pin_type, PinSubType pin_subtype) {
				
		auto input = std::make_unique<CorePin>(get_next_id(), this->id, pin_type, pin_subtype);
		
		inputs.push_back(std::move(input));
		
		return *inputs.back();
	}
	
	
	CorePin& add_output(PinType pin_type, PinSubType pin_subtype) {
		
		auto output = std::make_unique<CorePin>(get_next_id(), this->id, pin_type, pin_subtype);
		
		outputs.push_back(std::move(output));
		
		return *outputs.back();
	}
	
	void build();
	
	void reset_flow();
	
	const std::vector<std::unique_ptr<CorePin>>& get_inputs() {
		return inputs;
	}
	
	const std::vector<std::unique_ptr<CorePin>>& get_outputs() {
		return outputs;
	}
	
	void set_position(const nanogui::Vector2i& position) {
		this->position = position;
	}
	
	CorePin* find_pin(long long id) {
		// Search through input pins
		for (const auto& input : inputs) {
			if (input->id == id) {
				return input.get();
			}
		}
		
		// Search through output pins
		for (const auto& output : outputs) {
			if (output->id == id) {
				return output.get();
			}
		}
		
		// Pin not found
		return nullptr;
	}

		
private:
	long long get_next_id();
	long long next_id;
	
	std::vector<std::unique_ptr<CorePin>> inputs;
	std::vector<std::unique_ptr<CorePin>> outputs;
};

class DataCoreNode : public CoreNode {
public:
	DataCoreNode(NodeType type, long long id, nanogui::Color color = nanogui::Color(255, 255, 255, 255)) : CoreNode(type, id, color) {
		
	}
	virtual std::optional<std::variant<Entity, std::string, int, float, bool>> get_data() = 0;
	
	virtual void set_data(std::optional<std::variant<Entity, std::string, int, float, bool>> data) = 0;
};


class VisualLink;
class VisualBlueprintNode;

class VisualPin : public nanogui::ToolButton {
public:
	VisualPin(nanogui::Widget& parent, CorePin& core_pin);
	
	std::vector<VisualLink*> links;
	
	CoreNode* node() {
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
	
	
	VisualPin* find_pin(long long id) {
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
	std::vector<std::unique_ptr<nanogui::Widget>> data_widgets;
	
private:
	void create_visual_pins();
	void perform_layout(NVGcontext *ctx) override;
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

class BlueprintDataNode : public BlueprintNode {
public:
	BlueprintDataNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, NodeType type, const std::string& name, nanogui::Vector2i size, int id, nanogui::Color color = nanogui::Color(255, 255, 255, 255));
	
	virtual std::optional<std::variant<Entity, std::string, int, float, bool>> get_data() = 0;
	
	virtual void set_data(std::optional<std::variant<Entity, std::string, int, float, bool>> data) = 0;
};

class Link {
public:
	Link(int id, CorePin& start, CorePin& end)
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
