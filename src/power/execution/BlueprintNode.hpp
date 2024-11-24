#pragma once

#include "execution/BlueprintCanvas.hpp"

#include <nanogui/icons.h>
#include <nanogui/toolbutton.h>
#include <nanogui/vector.h>
#include <nanogui/textbox.h>
#include <nanogui/window.h>

#include <functional>

namespace blueprint {

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
	
private:
	std::optional<std::variant<Entity, std::string, int, float, bool>> mData;
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
	int id;
	nanogui::Color color;
	bool root_node = false;
	std::function<void()> link;
	std::function<void()> evaluate;
	
	BlueprintNode(std::optional<std::reference_wrapper<BlueprintCanvas>> parent, NodeType type, const std::string& name, nanogui::Vector2i size, int id, nanogui::Color color = nanogui::Color(255, 255, 255, 255));
	
	template<typename T, typename... Args>
	T& add_data_widget(Args&&... args){
		auto data_widget = std::make_unique<T>(*mDataColumn, std::forward<Args>(args)...);
		
		data_widget->set_fixed_size(nanogui::Vector2i(fixed_size().x() - 48, 22));
		
		auto& data_widget_ref = *data_widget;
		
		data_widgets.push_back(std::move(data_widget));
		
		return data_widget_ref;
	}
	
	
	template<typename T = Pin, typename... Args>
	Pin& add_input(int pin_id, int node_id, const std::string& label, PinType pin_type, PinSubType pin_subtype, Args &&...args) {
		
		auto& parent = pin_type == PinType::Flow ? *mFlowContainer : *mLeftColumn;
		
		auto input = std::make_unique<T>(parent, pin_id, node_id, label, pin_type, pin_subtype, std::forward<Args>(args)...);

		input->set_programmable(true);
		
		if (pin_type == PinType::Flow) {
			input->set_icon(FA_PLAY);
		}
		
		if (mCanvas.has_value()) {
			auto& input_ref = *input;
			
			input->set_callback([this, &input_ref](){
				mCanvas->get().on_input_pin_clicked(input_ref);
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
	Pin& add_output(int pin_id, int node_id, const std::string& label, PinType pin_type, PinSubType pin_subtype, Args &&...args) {
		auto& parent = pin_type == PinType::Flow ? *mFlowContainer : *mRightColumn;
		
		auto output = std::make_unique<T>(parent, pin_id, node_id, label, pin_type, pin_subtype, std::forward<Args>(args)...);
		
		output->set_programmable(true);
		
		if (pin_type == PinType::Flow) {
			output->set_icon(FA_PLAY);
		}
		
		if (mCanvas.has_value()) {
			auto& output_ref = *output;
			
			output->set_callback([this, &output_ref](){
				mCanvas->get().on_output_pin_clicked(output_ref);
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
	void perform_layout(NVGcontext *ctx) override;
	void draw(NVGcontext *ctx) override;
	std::optional<std::reference_wrapper<BlueprintCanvas>> mCanvas;
	std::unique_ptr<nanogui::Widget> mFlowContainer;
	std::unique_ptr<nanogui::Widget> mColumnContainer;
	std::unique_ptr<nanogui::Widget> mLeftColumn;
	std::unique_ptr<nanogui::Widget> mDataColumn;
	std::unique_ptr<nanogui::Widget> mRightColumn;
};


class Link : public nanogui::Widget {
public:
	Link(std::optional<std::reference_wrapper<blueprint::BlueprintCanvas>> parent, int id, Pin& start, Pin& end)
	: nanogui::Widget(parent), id(id), mStart(start), mEnd(end), color(nanogui::Color(255, 255, 255, 255)) {
		
	}
	
	Pin& get_start() const {
		return mStart;
	}
	
	Pin& get_end() const {
		return mEnd;
	}
	
	int get_id() const {
		return id;
	}

private:
	int id;
	Pin& mStart;
	Pin& mEnd;
	nanogui::Color color;
};

}
