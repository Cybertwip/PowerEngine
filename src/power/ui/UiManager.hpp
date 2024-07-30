#pragma once

#include "actors/IActorSelectedCallback.hpp"
#include "graphics/drawing/Drawable.hpp"
#include "graphics/shading/ShaderWrapper.hpp"

#include <optional>
#include <functional>

class IActorSelectedRegistry;
class ActorManager;
class ShaderManager;
class Canvas;

// UiManager class definition
class UiManager : public IActorSelectedCallback, public Drawable {
public:
	UiManager(IActorSelectedRegistry& registry, ActorManager& actorManager, ShaderManager& shaderManager, Canvas& canvas);
	~UiManager();
	
	void OnActorSelected(Actor& actor) override;
	
	void draw();
	
	void draw_content(const nanogui::Matrix4f& model, const nanogui::Matrix4f& view,
					  const nanogui::Matrix4f& projection) override;

private:
	IActorSelectedRegistry& mRegistry;
	ActorManager& mActorManager;
	std::optional<std::reference_wrapper<Actor>> mActiveActor;

	ShaderWrapper mShader;
};
