#include "SelfContainedMeshCanvas.hpp"
#include "components/SkinnedMeshComponent.hpp"
#include "components/MeshComponent.hpp"
#include "graphics/drawing/Batch.hpp"

#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/MeshBatch.hpp"
#include "graphics/drawing/SkinnedMeshBatch.hpp"

#if defined(NANOGUI_USE_METAL)
#include "MetalHelper.hpp"
#endif

#include <nanogui/screen.h>
#include <nanogui/renderpass.h>

namespace CanvasUtils {
nanogui::Matrix4f glm_to_nanogui(glm::mat4 glmMatrix) {
	nanogui::Matrix4f matrix;
	std::memcpy(matrix.m, glm::value_ptr(glmMatrix), sizeof(float) * 16);
	return matrix;
}
}

SelfContainedMeshCanvas::SelfContainedMeshCanvas(Widget* parent)
: nanogui::Canvas(parent, 1, true, true), mCurrentTime(0), mCamera(mRegistry), mShaderManager(*this),
mSkinnedMeshPreviewShader(*mShaderManager.load_shader("skinned_mesh_preview", "internal/shaders/metal/preview_diffuse_skinned_vs.metal",
	"internal/shaders/metal/preview_diffuse_fs.metal", nanogui::Shader::BlendMode::None)),
mUpdate(true) {
	set_background_color(nanogui::Color{70, 130, 180, 255});
	
	mCamera.add_component<TransformComponent>();
	mCamera.add_component<CameraComponent>(mCamera.get_component<TransformComponent>(), 45.0f, 0.01f, 5e3f, 192.0f / 128.0f);
	
	glm::quat rotationQuat = glm::angleAxis(glm::radians(180.0f), glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)));
	mModelMatrix = glm::identity<glm::mat4>() * glm::mat4_cast(rotationQuat);
	
	
	mMeshBatch = std::make_unique<MeshBatch>(*render_pass());
	
	mSkinnedMeshBatch = std::make_unique<SkinnedMeshBatch>(*render_pass());
	
	mBatchUnit = std::make_unique<BatchUnit>(*mMeshBatch, *mSkinnedMeshBatch);

}

void SelfContainedMeshCanvas::set_active_actor(std::optional<std::reference_wrapper<Actor>> actor) {
	clear();
	mCurrentTime = 0;
	mPreviewActor = actor;
	
	set_update(true);
	
	if (mPreviewActor.has_value()) {
		if (mPreviewActor->get().find_component<SkinnedAnimationComponent>()) {
			DrawableComponent& drawableComponent = mPreviewActor->get().get_component<DrawableComponent>();
			const SkinnedMeshComponent& meshComponent = static_cast<const SkinnedMeshComponent&>(drawableComponent.drawable());
			for (auto& skinnedData : meshComponent.get_skinned_mesh_data()) {
				
				mBatchUnit->mSkinnedMeshBatch.add_mesh(*skinnedData);
				
				mBatchUnit->mSkinnedMeshBatch.append(*skinnedData);
				
				mBatchPositions = mBatchUnit->mSkinnedMeshBatch.get_batch_positions();

			}
		} else {
			DrawableComponent& drawableComponent = mPreviewActor->get().get_component<DrawableComponent>();
			const MeshComponent& meshComponent = static_cast<const MeshComponent&>(drawableComponent.drawable());
			for (auto& meshData : meshComponent.get_mesh_data()) {
				
				mBatchUnit->mMeshBatch.add_mesh(*meshData);

				mBatchUnit->mMeshBatch.append(*meshData);
				
				mBatchPositions = mBatchUnit->mMeshBatch.get_batch_positions();
			}

		}
	} else {
		set_update(false);
	}
}

void SelfContainedMeshCanvas::draw_content(const nanogui::Matrix4f& view,
										   const nanogui::Matrix4f& projection) {
	mBatchUnit->mMeshBatch.draw_content(view, projection);
	
	mBatchUnit->mSkinnedMeshBatch.draw_content(view, projection);
}
}

void SelfContainedMeshCanvas::draw_contents() {
	if (mPreviewActor.has_value()) {
		auto& camera = mCamera.get_component<CameraComponent>();
		
		camera.update_view();
		
		auto& batchPositions = mBatchPositions;
		if (!batchPositions.empty()) {
			// Initialize min and max bounds with extreme values
			glm::vec3 minBounds(std::numeric_limits<float>::max());
			glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
			
			for (const auto& [identifier, positions] : batchPositions) {
				for (size_t i = 0; i < positions.size(); i += 3) {
					glm::vec3 pos(positions[i], positions[i + 1], positions[i + 2]);
					minBounds = glm::min(minBounds, pos);
					maxBounds = glm::max(maxBounds, pos);
				}
			}
			
			// Calculate center and size of the bounding box
			glm::vec3 center = (minBounds + maxBounds) * 0.5f;
			glm::vec3 size = maxBounds - minBounds;
			float distance = glm::length(size); // Camera distance from object based on size
			
			// Set the camera position and view direction
			auto& cameraTransform = mCamera.get_component<TransformComponent>();
			
			center.y = -center.y;
			
			cameraTransform.set_translation(center - glm::vec3(0.0f, 0.0f, distance));
			
			camera.look_at(mPreviewActor->get());
		}
		
		render_pass()->clear_color(0, nanogui::Color(0.0f, 0.0f, 0.0f, 1.0f));
		
		render_pass()->clear_depth(1.0f);
		
		render_pass()->set_depth_test(nanogui::RenderPass::DepthTest::Less, true);
		
		draw_content(camera.get_view(),
					 camera.get_projection());
	}
}

void SelfContainedMeshCanvas::clear() {

	mBatchUnit->mMeshBatch.clear();
	mBatchUnit->mSkinnedMeshBatch.clear();

}

void SelfContainedMeshCanvas::set_aspect_ratio(float ratio) {
	mCamera.get_component<CameraComponent>().set_aspect_ratio(ratio);
}
