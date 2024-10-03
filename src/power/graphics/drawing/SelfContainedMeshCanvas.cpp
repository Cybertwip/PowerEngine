#include "SelfContainedMeshCanvas.hpp"
#include "components/SkinnedMeshComponent.hpp"
#include "components/MeshComponent.hpp"
#include "graphics/drawing/Batch.hpp"

#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/SelfContainedMeshBatch.hpp"
#include "graphics/drawing/SelfContainedSkinnedMeshBatch.hpp"

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
// In SelfContainedMeshCanvas.cpp, modify the constructor to load and assign the mesh shader
SelfContainedMeshCanvas::SelfContainedMeshCanvas(Widget* parent)
: nanogui::Canvas(parent, 1, true, true), mCurrentTime(0), mCamera(mRegistry), mShaderManager(*this),
mSkinnedMeshPreviewShader(*mShaderManager.load_shader("skinned_mesh_preview",
													  "internal/shaders/metal/preview_diffuse_skinned_vs.metal",
													  "internal/shaders/metal/preview_diffuse_fs.metal",
													  nanogui::Shader::BlendMode::AlphaBlend)),
mMeshPreviewShader(*mShaderManager.load_shader("mesh_preview",
											  "internal/shaders/metal/preview_diffuse_vs.metal",
											  "internal/shaders/metal/preview_diffuse_fs.metal",
											  nanogui::Shader::BlendMode::AlphaBlend)),
mUpdate(true) {
	
	set_background_color(nanogui::Color{70, 130, 180, 255});
	
	mCamera.add_component<TransformComponent>();
	mCamera.add_component<CameraComponent>(mCamera.get_component<TransformComponent>(),
										   45.0f, 0.01f, 5e3f, 192.0f / 128.0f);
	
	glm::quat rotationQuat = glm::angleAxis(glm::radians(180.0f),
											glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)));
	mModelMatrix = glm::identity<glm::mat4>() * glm::mat4_cast(rotationQuat);
	
	// Initialize MeshBatch with its shader
	mMeshBatch = std::make_unique<SelfContainedMeshBatch>(*render_pass(), mMeshPreviewShader);
	
	// Initialize SkinnedMeshBatch with its shader
	mSkinnedMeshBatch = std::make_unique<SelfContainedSkinnedMeshBatch>(*render_pass(), mSkinnedMeshPreviewShader);
}
void SelfContainedMeshCanvas::set_active_actor(std::optional<std::reference_wrapper<Actor>> actor) {
	clear();
	mCurrentTime = 0;

	mPreviewActor = actor;
	
	if (mPreviewActor.has_value()) {
		DrawableComponent& drawableComponent = mPreviewActor->get().get_component<DrawableComponent>();
		const Drawable& drawableRef = drawableComponent.drawable();
		
		// Attempt to cast to SkinnedMeshComponent
		const SkinnedMeshComponent* skinnedMeshComponent = dynamic_cast<const SkinnedMeshComponent*>(&drawableRef);
		
		if (skinnedMeshComponent) {
			// Successfully cast to SkinnedMeshComponent
			for (auto& skinnedData : skinnedMeshComponent->get_skinned_mesh_data()) {
				mSkinnedMeshBatch->add_mesh(*skinnedData);
				mSkinnedMeshBatch->append(*skinnedData);
				mBatchPositions = mSkinnedMeshBatch->get_batch_positions();
			}
			
			update_camera_view();
			
			set_update(true);
		}
		else {
			// Attempt to cast to MeshComponent
			const MeshComponent* meshComponent = dynamic_cast<const MeshComponent*>(&drawableRef);
			if (meshComponent) {
				// Successfully cast to MeshComponent
				for (auto& meshData : meshComponent->get_mesh_data()) {
					mMeshBatch->add_mesh(*meshData);
					mMeshBatch->append(*meshData);
					mBatchPositions = mMeshBatch->get_batch_positions();
				}
				
				update_camera_view();
				set_update(true);

			}
			else {
				// Handle the case where drawable is neither SkinnedMeshComponent nor MeshComponent
				std::cerr << "Error: DrawableComponent does not contain a SkinnedMeshComponent or MeshComponent." << std::endl;
				set_update(false);
				return;
			}
		}
	}
	else {
		set_update(false);
	}
}

void SelfContainedMeshCanvas::update_camera_view() {
	
	auto& batchPositions = mBatchPositions;
	if (!batchPositions.empty()) {
		auto& camera = mCamera.get_component<CameraComponent>();

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
		
		camera.look_at(glm::vec3(0.0f, 0.0f, 0.0f));
	}
}

void SelfContainedMeshCanvas::draw_content(const nanogui::Matrix4f& view,
										   const nanogui::Matrix4f& projection) {

	DrawableComponent& drawableComponent = mPreviewActor->get().get_component<DrawableComponent>();
	
	Drawable& drawableRef = drawableComponent.drawable();
	
	
	if (mPreviewActor->get().find_component<SkinnedAnimationComponent>()) {
		SkinnedAnimationComponent& animationComponent = mPreviewActor->get().get_component<SkinnedAnimationComponent>();
		
		animationComponent.evaluate_time(mCurrentTime, SkinnedAnimationComponent::PlaybackModifier::Forward);
		
		mCurrentTime += 1;
	}
	
	drawableRef.draw_content(CanvasUtils::glm_to_nanogui(mModelMatrix), view, projection);
	
	mMeshBatch->draw_content(view, projection);
	
	mSkinnedMeshBatch->draw_content(view, projection);
}

void SelfContainedMeshCanvas::draw_contents() {
	if (mPreviewActor.has_value() && mUpdate) {
		auto& camera = mCamera.get_component<CameraComponent>();
		
		camera.update_view();
		
		render_pass()->clear_color(0, nanogui::Color(0.0f, 0.0f, 0.0f, 1.0f));
		
		render_pass()->clear_depth(1.0f);
		
		render_pass()->set_depth_test(nanogui::RenderPass::DepthTest::Less, true);
		
		draw_content(camera.get_view(),
					 camera.get_projection());
	}
}

void SelfContainedMeshCanvas::clear() {
	set_update(false);

	mMeshBatch->clear();
	mSkinnedMeshBatch->clear();
}

void SelfContainedMeshCanvas::set_aspect_ratio(float ratio) {
	mCamera.get_component<CameraComponent>().set_aspect_ratio(ratio);
}
