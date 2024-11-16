#pragma once

#include <nanogui/icons.h>
#include <nanogui/toolbutton.h>
#include <nanogui/vector.h>
#include <nanogui/widget.h>
#include <nanogui/window.h>
#include <nanovg.h>

namespace blueprint {


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

struct Node;
struct Link;

struct Entity {
	int id;
	std::optional<std::variant<std::string, int, float, bool>> payload;
};

class Pin : public nanogui::ToolButton {
public:
	int id;
	int node_id;
	Node* node;
	PinType type;
	PinSubType subtype;
	PinKind kind;
	bool can_flow = false;
	std::optional<std::variant<Entity, std::string, int, float, bool>> data;
	
	Pin(nanogui::Widget& parent, int id, int node_id, const std::string& label, PinType type, PinSubType subtype = PinSubType::None)
	: nanogui::ToolButton(parent, FA_CIRCLE_NOTCH, label), id(id), node_id(node_id), node(nullptr), type(type), subtype(subtype), kind(PinKind::Input) {
		if (type == PinType::Bool) {
			data = true;
		} else if (type == PinType::String) {
			data = "";
		} else if (type == PinType::Float) {
			data = 0.0f;
		} else if (type == PinType::Flow) {
			data = true;
		}
	}
};

struct Link {
	int id;
	int start_pin_id;
	int end_pin_id;
	nanogui::Color color;
	
	Link(int id, int start_pin_id, int end_pin_id)
	: id(id), start_pin_id(start_pin_id), end_pin_id(end_pin_id), color(nanogui::Color(255, 255, 255, 255)) {}
};


class Node : public nanogui::Window {
public:
	int id;
	nanogui::Color color;
	bool root_node = false;
	std::function<void()> link;
	std::function<void()> evaluate;
	
	Node(nanogui::Widget& parent, const std::string& name, nanogui::Vector2i size, int id, nanogui::Color color = nanogui::Color(255, 255, 255, 255));
	
	template<typename T, typename... Args>
	T& add_data_widget(Args&&... args){
		auto data_widget = std::make_unique<T>(*mDataColumn, std::forward<Args>(args)...);
		
		data_widget->set_fixed_size(nanogui::Vector2i(fixed_size().x() - 48, 22));
		
		auto& data_widget_ref = *data_widget;
		
		data_widgets.push_back(std::move(data_widget));
		
		return data_widget_ref;
	}
	
	
	Pin& add_input(int pin_id, int node_id, const std::string& label, PinType pin_type);
	
	Pin& add_output(int pin_id, int node_id, const std::string& label, PinType pin_type);
	
	void build();
	
	void reset_flow();
	
protected:
	std::vector<std::unique_ptr<Pin>> inputs;
	std::vector<std::unique_ptr<nanogui::Widget>> data_widgets;
	std::vector<std::unique_ptr<Pin>> outputs;
	
private:
	void perform_layout(NVGcontext *ctx) override;
	void draw(NVGcontext *ctx) override;
	
	std::unique_ptr<nanogui::Widget> mFlowContainer;
	std::unique_ptr<nanogui::Widget> mColumnContainer;
	std::unique_ptr<nanogui::Widget> mLeftColumn;
	std::unique_ptr<nanogui::Widget> mDataColumn;
	std::unique_ptr<nanogui::Widget> mRightColumn;
};

}
