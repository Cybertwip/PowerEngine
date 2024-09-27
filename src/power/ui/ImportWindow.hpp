#pragma once

#include <entt/entt.hpp>

#include <nanogui/window.h>

#include <memory>

class MeshActorBuilder;
class MeshActorImporter;
class MeshBatch;
class ShaderWrapper;
class ShaderManager;
class SharedSelfContainedMeshCanvas;
class SkinnedMeshBatch;


struct BatchUnit;

namespace nanogui {
class RenderPass;
}

class ImportWindow : public nanogui::Window {
public:
	ImportWindow(nanogui::Widget* parent, nanogui::RenderPass& renderpass, ShaderManager& shaderManager);
	
	void Preview(const std::string& path, const std::string& directory);

private:
	void ImportIntoProject(const std::string& path);
	
	std::unique_ptr<MeshBatch> mMeshBatch;
	std::unique_ptr<SkinnedMeshBatch> mSkinnedMeshBatch;
	
	std::unique_ptr<BatchUnit> mBatchUnit;

	std::unique_ptr<MeshActorBuilder> mMeshActorBuilder;

	SharedSelfContainedMeshCanvas* mPreviewCanvas;
	
	std::unique_ptr<ShaderWrapper> mMeshShader;
	std::unique_ptr<ShaderWrapper> mSkinnedShader;


	entt::registry mDummyRegistry;

	std::unique_ptr<MeshActorImporter> mMeshActorImporter;

};
