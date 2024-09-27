#pragma once
#include <nanogui/canvas.h>
#include <nanogui/vector.h>
#include <nanogui/texture.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <entt/entt.hpp>
#include <unordered_map>
#include <optional>
#include "actors/Actor.hpp"
#include "ShaderManager.hpp"
#include "SkinnedMesh.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/SkinnedAnimationComponent.hpp"

namespace CanvasUtils {
nanogui::Matrix4f glm_to_nanogui(glm::mat4 glmMatrix);
}

class SelfContainedMeshCanvas : public nanogui::Canvas {
private:
	struct VertexIndexer {
		size_t mVertexOffset = 0;
		size_t mIndexOffset = 0;
	};

public:
	SelfContainedMeshCanvas(Widget* parent);
	void set_active_actor(std::optional<std::reference_wrapper<Actor>> actor);
	
	void set_update(bool update) {
		mUpdate = update;
	}

private:
	void add_mesh(std::reference_wrapper<SkinnedMesh> mesh);
	void append(std::reference_wrapper<SkinnedMesh> meshRef);
	void draw_content(const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection);
	void draw_contents() override;
	void clear();
	void upload_vertex_data(ShaderWrapper& shader, int identifier);
	void upload_material_data(ShaderWrapper& shader, const std::vector<std::shared_ptr<MaterialProperties>>& materialData);
	
	std::optional<std::reference_wrapper<Actor>> mPreviewActor;
	entt::registry mRegistry;
	int mCurrentTime;
	Actor mCamera;
	ShaderManager mShaderManager;
	ShaderWrapper mSkinnedMeshPreviewShader;
	
	std::unordered_map<ShaderWrapper*, std::vector<std::reference_wrapper<SkinnedMesh>>> mMeshes;
	std::unordered_map<int, std::vector<float>> mBatchPositions;
	std::unordered_map<int, std::vector<float>> mBatchTexCoords1;
	std::unordered_map<int, std::vector<float>> mBatchTexCoords2;
	std::unordered_map<int, std::vector<int>> mBatchTextureIds;
	std::unordered_map<int, std::vector<int>> mBatchBoneIds;
	std::unordered_map<int, std::vector<float>> mBatchBoneWeights;
	std::unordered_map<int, std::vector<unsigned int>> mBatchIndices;
	std::vector<std::shared_ptr<MaterialProperties>> mBatchMaterials;
	
	std::unordered_map<int, std::vector<size_t>> mMeshStartIndices;
	std::unordered_map<int, VertexIndexer> mVertexIndexingMap;
	glm::mat4 mModelMatrix;
	
	bool mUpdate;
};
