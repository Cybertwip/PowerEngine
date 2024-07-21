#include <entt/entt.hpp>

#include <nanogui/screen.h>

#include <memory>
#include <vector>

#include "graphics/drawing/SkinnedMesh.hpp"

class Actor;
class ActorManager;
class Canvas;
class CameraManager;
class MeshActor;
class MeshActorLoader;
class RenderCommon;
class RenderSettings;
class SkinnedMesh;
class ShaderManager;
class ShaderWrapper;
class UiCommon;

class Application : public nanogui::Screen
{
   public:
    Application();

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;
    virtual void draw(NVGcontext *ctx) override;

   private:
    std::unique_ptr<entt::registry> mEntityRegistry;
    std::unique_ptr<CameraManager> mCameraManager;
    std::unique_ptr<Canvas> mCanvas;
    std::unique_ptr<ShaderManager> mShaderManager;
    std::unique_ptr<RenderSettings> mRenderSettings;
    std::unique_ptr<UiCommon> mUiCommon;
    std::unique_ptr<RenderCommon> mRenderCommon;
    std::unique_ptr<ActorManager> mActorManager;
    
    std::vector<std::reference_wrapper<Actor>> mActors;

};
