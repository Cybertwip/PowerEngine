#include "graphics/drawing/MeshActorBuilder.hpp"

#include <filesystem>

#include "actors/Actor.hpp"

#include "components/ColorComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/MetadataComponent.hpp"
#include "components/TransformComponent.hpp"
#include "import/Fbx.hpp"

Actor& MeshActorBuilder::build(Actor& actor, const std::string& path,
                     SkinnedMesh::SkinnedMeshShader& meshShaderWrapper) {
    auto model = Fbx(path);

    std::vector<std::unique_ptr<SkinnedMesh>> meshComponentData;

    for (auto& meshData : model.GetMeshData()) {
        meshComponentData.push_back(std::make_unique<SkinnedMesh>(std::move(meshData), meshShaderWrapper));
    }

	std::unique_ptr<Drawable> meshComponent = std::make_unique<MeshComponent>(meshComponentData);
	actor.add_component<DrawableComponent>(std::move(meshComponent));
	actor.add_component<TransformComponent>();
	actor.add_component<MetadataComponent>(actor.identifier(), std::filesystem::path(path).stem().string());
	actor.add_component<ColorComponent>(actor.get_component<MetadataComponent>(), meshShaderWrapper);

	return actor;
}
