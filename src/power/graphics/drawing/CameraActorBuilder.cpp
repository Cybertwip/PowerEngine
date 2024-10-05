#include "graphics/drawing/CameraActorBuilder.hpp"
#include "graphics/drawing/NullDrawable.hpp"


#include "actors/Actor.hpp"

#include "components/AnimationComponent.hpp"
#include "components/CameraComponent.hpp"
#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
//#include "components/MeshComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/TimelineComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/TransformAnimationComponent.hpp"

Actor& CameraActorBuilder::build(Actor& actor,
								 AnimationTimeProvider& animationTimeProvider,
								 ShaderWrapper& meshShaderWrapper, float fov, float near, float far, float aspect) {
	std::unique_ptr<Drawable> drawable = std::make_unique<NullDrawable>();
	actor.add_component<DrawableComponent>(std::move(drawable));
	auto& transform = actor.add_component<TransformComponent>();
	actor.add_component<MetadataComponent>(actor.identifier(), "Camera");
	actor.add_component<ColorComponent>(actor.get_component<MetadataComponent>());
	actor.add_component<CameraComponent>(actor.get_component<TransformComponent>(), fov, near, far, aspect);
	
	auto& transformAnimationComponent = actor.add_component<TransformAnimationComponent>(transform, animationTimeProvider);
	
	std::vector<std::reference_wrapper<AnimationComponent>> timelineComponents = {
		transformAnimationComponent
	};
	
	actor.add_component<TimelineComponent>(std::move(timelineComponents), animationTimeProvider);
	
	return actor;
}
