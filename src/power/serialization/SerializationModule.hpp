#pragma once

#include <memory>

class ActorManager;
class AnimationTimeProvider;
class MeshActorBuilder;
class ShaderWrapper;
class SceneSerializer;

class SerializationModule {
public:
	SerializationModule(ActorManager& actorManager, MeshActorBuilder& actorBuilder, AnimationTimeProvider& timeProvider, ShaderWrapper& meshShader, ShaderWrapper& skinnedShader);
	
	void save_scene(const std::string& filepath);
	
	void load_scene(const std::string& filepath);

	
private:
	ActorManager& mActorManager;
	MeshActorBuilder& mMeshActorBuilder;
	AnimationTimeProvider& mTimeProvider;
	ShaderWrapper& mMeshShader;
	ShaderWrapper& mSkinnedMeshShader;
	std::unique_ptr<SceneSerializer> mSceneSerializer;
};
