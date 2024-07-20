#include <entt/entt.hpp>

#include <nanogui/screen.h>

#include <memory>
#include <vector>

#include "graphics/drawing/SkinnedMesh.hpp"

class Canvas;
class CameraManager;
class MeshActor;
class MeshActorLoader;
class RenderManager;
class SkinnedMesh;
class ShaderManager;
class ShaderWrapper;

class Application : public nanogui::Screen
{
   public:
    Application();

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;
    virtual void draw(NVGcontext *ctx) override;

   private:
    std::unique_ptr<entt::registry> mEntityRegistry;
    std::unique_ptr<CameraManager> mCameraManager;
    std::unique_ptr<RenderManager> mRenderManager;
    std::unique_ptr<Canvas> mCanvas;
    std::unique_ptr<ShaderManager> mShaderManager;
    std::unique_ptr<MeshActorLoader> mMeshActorLoader;

    std::vector<std::unique_ptr<MeshActor>> mActors;
};
