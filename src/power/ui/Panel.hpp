#pragma once

#include <nanogui/window.h>

class Panel : public nanogui::Window {
public:
    Panel(nanogui::Widget& parent, const std::string &title = "");

protected:
    virtual bool mouse_drag_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
                                  int button, int modifiers) override;
	void initialize() override;

private:
};
