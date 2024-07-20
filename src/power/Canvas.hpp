#pragma once

#include <nanogui/canvas.h>

class RenderManager;

class Canvas : public nanogui::Canvas {
public:
	Canvas(Widget *parent, RenderManager& renderManager, nanogui::Color backgroundColor, nanogui::Vector2i size);
	
	virtual void draw_contents() override;
	
private:
	void visit(RenderManager& renderManager);
    
    RenderManager& mRenderManager;
};
