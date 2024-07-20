#pragma once

#include <nanogui/canvas.h>

class RenderManager;

class Canvas : public nanogui::Canvas {
public:
	Canvas(nanogui::Widget *parent, RenderManager& renderManager);
	
	virtual void draw_contents() override;
	
private:
	void visit(RenderManager& renderManager);
    
    RenderManager& mRenderManager;
};
