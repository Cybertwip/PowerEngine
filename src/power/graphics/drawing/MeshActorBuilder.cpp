#include "graphics/drawing/MeshActorBuilder.hpp"

#include <filesystem>
#include <functional>

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
	actor.add_component<MetadataComponent>(std::filesystem::path(path).stem().string());
		
	return actor;
}
