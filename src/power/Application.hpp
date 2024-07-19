#include "graphics/drawing/SkinnedMesh.hpp"

#include <nanogui/screen.h>

#include <memory>

class Canvas;
class SkinnedMesh;
class ShaderManager;
class ShaderWrapper;

class Application : public nanogui::Screen {
public:
	Application();

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;
    virtual void draw(NVGcontext *ctx) override;

private:
	std::unique_ptr<ShaderManager> mShaderManager;
	std::unique_ptr<SkinnedMesh::SkinnedMeshShader> mMeshShaderWrapper;

    Canvas* mCanvas;
	SkinnedMesh* mMesh;
};
