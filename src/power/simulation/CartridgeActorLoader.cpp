#include "simulation/CartridgeActorLoader.hpp"

#include "simulation/VirtualMachine.hpp"

#include "actors/IActorSelectedRegistry.hpp"
#include "actors/ActorManager.hpp"

#include "MeshActorLoader.hpp"

CartridgeActorLoader::CartridgeActorLoader(VirtualMachine& virtualMachine, MeshActorLoader& meshActorLoader, IActorManager& actorManager, IActorVisualManager& actorVisualManager, AnimationTimeProvider& animationTimeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader) :
mVirtualMachine(virtualMachine)
, mMeshActorLoader(meshActorLoader)
, mActorManager(actorManager)
, mActorVisualManager(actorVisualManager)
, mAnimationTimeProvider(animationTimeProvider)
, mMeshShader(meshShader)
, mSkinnedMeshShader(skinnedShader)
{
	
	// Register HostClass::multiply
	mVirtualMachine.register_callback(reinterpret_cast<uint64_t>(this), "CartridgeActorLoader::create_actor",
						 [](uint64_t this_ptr, const std::vector<std::any>& args, std::vector<unsigned char>& output_buffer) {
		if (args.size() != 1) {
			throw std::runtime_error("Argument count mismatch for multiply");
		}
		PrimitiveShape primitiveShape = static_cast<PrimitiveShape>(std::any_cast<uint64_t>(args[0]));
		auto& instance = *reinterpret_cast<CartridgeActorLoader*>(this_ptr);
		
		uint64_t result = reinterpret_cast<uint64_t>(instance.create_actor(primitiveShape));
		
		// Copy result into output_buffer
		unsigned char* data_ptr = reinterpret_cast<unsigned char*>(&result);
		std::memcpy(output_buffer.data(), data_ptr, sizeof(uint64_t));
	});
}

void CartridgeActorLoader::cleanup() {
	mActorVisualManager.remove_actors(mLoadedActors);
	mLoadedActors.clear();
}

Actor& CartridgeActorLoader::create_actor(const std::string& filePath) {
	Actor& actor = mMeshActorLoader.create_actor(filePath, mAnimationTimeProvider, mMeshShader, mSkinnedMeshShader);
	
	mActorVisualManager.add_actor(actor);
	
	mLoadedActors.push_back(actor);
	
	return actor;
}

Primitive* CartridgeActorLoader::create_actor(PrimitiveShape primitiveShape) {
	Actor& actor = mMeshActorLoader.create_actor("Primitive" + std::to_string(mLoadedPrimitives.size()), primitiveShape, mMeshShader);
	
	mActorVisualManager.add_actor(actor);
	
	mLoadedActors.push_back(actor);

	mLoadedPrimitives.push_back(std::make_unique<Primitive>(actor));
	
	// register here
	mVirtualMachine.register_callback(reinterpret_cast<uint64_t>(mLoadedPrimitives.back().get()), "Primitive::get_translation",
						 [](uint64_t this_ptr, const std::vector<std::any>& args, std::vector<unsigned char>& output_buffer) {
		if (!args.empty()) {
			throw std::runtime_error("Argument count mismatch for get_translation");
		}
		auto& instance = *reinterpret_cast<Primitive*>(this_ptr);
		glm::vec3 result = instance.get_translation();
		
		// Copy result into output_buffer
		unsigned char* data_ptr = reinterpret_cast<unsigned char*>(&result);
		std::memcpy(output_buffer.data(), data_ptr, sizeof(glm::vec3));
	});
	
	// Register HostDummyMath::set
	mVirtualMachine.register_callback(reinterpret_cast<uint64_t>(mLoadedPrimitives.back().get()), "Primitive::set_translation",
						 [](uint64_t this_ptr, const std::vector<std::any>& args, std::vector<unsigned char>& output_buffer) {
		auto& instance = *reinterpret_cast<Primitive*>(this_ptr);
		
		if (!args.empty()) {
			throw std::runtime_error("Argument count mismatch for set_translation");
		}
		
		glm::vec3 data;
		std::memcpy(&data, output_buffer.data(), sizeof(glm::vec3));
		
		instance.set_translation(data);
	});


	return mLoadedPrimitives.back().get();
}





