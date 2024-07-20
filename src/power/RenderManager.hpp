#pragma once

#include <functional>
#include <vector>

class Canvas;
class Drawable;
class CameraManager;

class RenderManager
{
   public:
    RenderManager(CameraManager& cameraManager);
    
    void add_drawable(std::reference_wrapper<Drawable> drawable);

    void render(Canvas& canvas);

   private:
    CameraManager& mCameraManager;
    std::vector<std::reference_wrapper<Drawable>> drawables;
};
