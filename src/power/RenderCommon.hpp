#pragma once

#include <nanogui/widget.h>

#include <entt/entt.hpp>

#include <memory>

namespace nanogui{
class Widget;
}

class ActorManager;
class CameraActorLoader;
class CameraManager;
class Canvas;
class GizmoManager;
class MeshActorLoader;
class ShaderManager;

class RenderSettings {
public:
    RenderSettings(int canvasWidth, int canvasHeight) :
    mCanvasWidth(canvasWidth),
    mCanvasHeight(canvasHeight){
        
    }
    int mCanvasWidth;
    int mCanvasHeight;
};

class RenderCommon : public nanogui::Widget {
public:
    RenderCommon(nanogui::Widget& parent, nanogui::Screen& screen, entt::registry& registry, ActorManager& actorManager, CameraManager& cameraManager);
	
	std::shared_ptr<Canvas> canvas() {
        return mCanvas;
    }

    ShaderManager& shader_manager() {
        return *mShaderManager;
    }
    
	CameraActorLoader& camera_actor_loader() {
		return *mCameraActorLoader;
	}
	
	void initialize() override;

private:
	
	std::shared_ptr<Canvas> mCanvas;
    std::unique_ptr<ShaderManager> mShaderManager;
	std::unique_ptr<CameraActorLoader> mCameraActorLoader;
	
	ActorManager& mActorManager;
	CameraManager& mCameraManager;
};

