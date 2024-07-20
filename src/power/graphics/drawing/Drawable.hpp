#pragma once

class Canvas;
class CameraManager;

class Drawable {
public:
	virtual ~Drawable() = default;
	virtual void draw_content(CameraManager& cameraManager) = 0;
};
