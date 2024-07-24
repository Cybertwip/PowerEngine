#include "MeshActorLoader.hpp"

#include "components/MeshComponent.hpp"
#include "graphics/drawing/MeshActor.hpp"
#include "import/Fbx.hpp"

MeshActorLoader::MeshActorLoader(entt::registry& registry, ShaderManager& shaderManager)
    : mEntityRegistry(registry),
      mMeshShaderWrapper(std::make_unique<SkinnedMesh::SkinnedMeshShader>(shaderManager)) {}

Actor& MeshActorLoader::create_mesh_actor(const std::string& path) {
	auto deleter = [this](Actor* ptr) {
		
		auto existing = std::find_if(mActors.begin(), mActors.end(), [this, ptr](const auto& actor){
			if (ptr == actor.get()) {
				return true;
			} else {
				return false;
			}
		});
		
		assert(existing != mActors.end());
		
		mActors.erase(existing);
		
		delete ptr;
	};
	
	mActors.emplace_back(std::move(
								   std::move(std::unique_ptr<MeshActor, decltype(deleter)>(new MeshActor(mEntityRegistry, path, *mMeshShaderWrapper), deleter))));

	
    return *mActors.back();
}
