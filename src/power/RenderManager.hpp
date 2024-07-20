#pragma once

#include <functional>
#include <vector>

class Canvas;
class Drawable;

class RenderManager {
public:
    void add_drawable(std::reference_wrapper<Drawable> drawable);
    
    void render(Canvas& canvas);
    
private:
    std::vector<std::reference_wrapper<Drawable>> drawables;
};
