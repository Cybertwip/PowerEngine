#include "graphics/drawing/MeshActorBuilder.hpp"

#include <filesystem>

#include "actors/Actor.hpp"

#include "components/AnimationComponent.hpp"
#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/TransformComponent.hpp"

#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include "import/Fbx.hpp"

MeshActorBuilder::MeshActorBuilder(BatchUnit& batchUnit)
: mBatchUnit(batchUnit) {
	
}

Actor& MeshActorBuilder::build(Actor& actor, const std::string& path, ShaderWrapper& shader) {
	
	actor.add_component<MetadataComponent>(actor.identifier(), std::filesystem::path(path).stem().string());

	auto& colorComponent = actor.add_component<ColorComponent>(actor.get_component<MetadataComponent>());

    auto model = Fbx(path);

    std::vector<std::unique_ptr<Mesh>> meshComponentData;

    for (auto& meshData : model.GetMeshData()) {
        meshComponentData.push_back(std::make_unique<Mesh>(std::move(meshData), shader, mBatchUnit.mMeshBatch, colorComponent));
    }

	std::unique_ptr<Drawable> meshComponent = std::make_unique<MeshComponent>(meshComponentData);
	actor.add_component<DrawableComponent>(std::move(meshComponent));
	actor.add_component<TransformComponent>();
	actor.add_component<AnimationComponent>();

	return actor;
}
