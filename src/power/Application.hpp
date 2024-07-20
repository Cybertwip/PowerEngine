#include "graphics/drawing/SkinnedMesh.hpp"

#include <nanogui/screen.h>

#include <vector>
#include <memory>

class Canvas;
class RenderManager;
class SkinnedMesh;
class ShaderManager;
class ShaderWrapper;

class Fbx;

class Application : public nanogui::Screen {
public:
	Application();

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;
    virtual void draw(NVGcontext *ctx) override;

private:
    std::unique_ptr<RenderManager> mRenderManager;
    std::unique_ptr<ShaderManager> mShaderManager;
	std::unique_ptr<SkinnedMesh::SkinnedMeshShader> mMeshShaderWrapper;

    std::unique_ptr<Fbx> mModel;

	std::vector<std::unique_ptr<SkinnedMesh>> mMeshes;
};
