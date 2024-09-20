#pragma once 
#include <entt/entt.hpp>

#include <nanogui/screen.h>

#include <memory>
#include <vector>

class Actor;
class ActorManager;
class Batch;
class Canvas;
class CameraManager;
class GizmoManager;
class MeshActor;
class MeshActorLoader;
class RenderCommon;
class ShaderManager;
class ShaderWrapper;
class UiCommon;
class UiManager;
class MeshBatch;

class Application : public nanogui::Screen
{
   public:
    Application();

    virtual bool keyboard_event(int key, int scancode, int action, int modifiers) override;
	virtual void draw(NVGcontext *ctx) override;
	virtual void process_events() override;

	void register_click_callback(std::function<void(bool, int, int, int, int)> callback);

   private:
	
	bool mouse_button_event(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

    std::unique_ptr<entt::registry> mEntityRegistry;
    std::unique_ptr<CameraManager> mCameraManager;
    std::unique_ptr<Canvas> mCanvas;
	std::unique_ptr<ActorManager> mActorManager;
	std::unique_ptr<RenderCommon> mRenderCommon;
	std::unique_ptr<MeshActorLoader> mMeshActorLoader;
    std::unique_ptr<UiCommon> mUiCommon;
	std::unique_ptr<UiManager> mUiManager;
	
	std::unique_ptr<MeshBatch> mMeshBatch;
	
	std::vector<std::reference_wrapper<Batch>> mBatchUnit;


	std::queue<std::tuple<bool, int, int, int, int>> mClickQueue;
	std::vector<std::function<void(bool, int, int, int, int)>> mClickCallbacks;

};
