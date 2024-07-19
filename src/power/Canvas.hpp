#pragma once

#include <nanogui/canvas.h>
#include <vector>
#include <functional>

class Drawable;

class Canvas : public nanogui::Canvas {
public:
	Canvas(nanogui::Widget *parent);
	
	virtual void draw_contents() override;
	
	void add_drawable(std::reference_wrapper<Drawable> drawable);
	
private:
	std::vector<std::reference_wrapper<Drawable>> drawables;
};
