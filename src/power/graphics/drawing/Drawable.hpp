#pragma once

class Canvas;

class Drawable {
public:
	virtual ~Drawable() = default;
	virtual void draw_content(Canvas& canvas) = 0;
};
