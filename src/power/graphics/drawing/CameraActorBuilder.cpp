#include "graphics/drawing/CameraActorBuilder.hpp"
#include "graphics/drawing/NullDrawable.hpp"


#include "actors/Actor.hpp"

#include "components/AnimationComponent.hpp"
#include "components/CameraComponent.hpp"
#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
//#include "components/MeshComponent.hpp"
#include "components/MetadataComponent.hpp"

#include "components/TransformComponent.hpp"
#include "components/TransformAnimationComponent.hpp"

AnimationTimeProvider CameraActorBuilder::mDummyAnimationTimeProvider  (60 * 30);

Actor& CameraActorBuilder::build(Actor& actor,
                     ShaderWrapper& meshShaderWrapper, float fov, float near, float far, float aspect) {
	std::unique_ptr<Drawable> drawable = std::make_unique<NullDrawable>();
	actor.add_component<DrawableComponent>(std::move(drawable));
	auto& transform = actor.add_component<TransformComponent>();
	actor.add_component<MetadataComponent>(actor.identifier(), "Camera");
	actor.add_component<ColorComponent>(actor.get_component<MetadataComponent>());
	actor.add_component<CameraComponent>(actor.get_component<TransformComponent>(), fov, near, far, aspect);
	
	actor.add_component<TransformAnimationComponent>(transform, CameraActorBuilder::mDummyAnimationTimeProvider);
	
	return actor;
}
