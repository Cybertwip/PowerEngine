#pragma once
#include "actors/Actor.hpp"
#include "ShaderManager.hpp"
#include "SkinnedMesh.hpp"
#include "graphics/shading/MaterialProperties.hpp"
#include "graphics/drawing/SelfContainedMeshBatch.hpp"
#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/DrawableComponent.hpp"
#include "components/SkinnedAnimationComponent.hpp"

#include <nanogui/canvas.h>
#include <nanogui/vector.h>
#include <nanogui/texture.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <entt/entt.hpp>

#include <unordered_map>
#include <optional>

#include <mutex>

class SelfContainedSkinnedMeshBatch;
class SelfContainedMeshBatch;

namespace CanvasUtils {
nanogui::Matrix4f glm_to_nanogui(glm::mat4 glmMatrix);
}

class SelfContainedMeshCanvas : public nanogui::Canvas {
private:
	struct VertexIndexer {
		unsigned int mVertexOffset = 0;
		unsigned int mIndexOffset = 0;
	};

public:
	SelfContainedMeshCanvas(nanogui::Widget& parent, nanogui::Screen& screen, AnimationTimeProvider& previewTimeProvider);
	void set_active_actor(std::optional<std::reference_wrapper<Actor>> actor);
	
	void set_update(bool update) {
		nanogui::async([this, update]{
			std::unique_lock<std::mutex> lock(mUpdateMutex);
			mUpdate = update;
		});
	}
	
	void set_aspect_ratio(float ratio);
	
	void rotate(const glm::vec3& axis, float angle);
	
	ShaderWrapper& get_mesh_shader() {
		return *mMeshPreviewShader;
	}
	ShaderWrapper& get_skinned_mesh_shader() {
		return *mSkinnedMeshPreviewShader;
	}
	
protected:
	void clear();

	void draw_contents() override;

private:	
	void update_camera_view();
	
	void draw_content(const nanogui::Matrix4f& view, const nanogui::Matrix4f& projection);
	
	AnimationTimeProvider& mPreviewTimeProvider;
	
	std::optional<std::reference_wrapper<Actor>> mPreviewActor;
	entt::registry mRegistry;
	int mCurrentTime;
	Actor mCamera;
	std::optional<ShaderManager> mShaderManager;
	std::optional<ShaderWrapper> mMeshPreviewShader;
	std::optional<ShaderWrapper> mSkinnedMeshPreviewShader;

	glm::vec3 mModelMinimumBounds;
	glm::vec3 mModelMaximumBounds;

	glm::mat4 mModelMatrix;
	
	bool mUpdate;
	
	std::unique_ptr<SelfContainedMeshBatch> mMeshBatch;
	std::unique_ptr<SelfContainedSkinnedMeshBatch> mSkinnedMeshBatch;
	
	std::mutex mMutex;
	
	std::mutex mUpdateMutex;
};
