#pragma once

class Drawable {
public:
	virtual ~Drawable() = default;
	virtual void draw_content() = 0;
};
