#include "SelfContainedMeshCanvas.hpp"
#include "components/SkinnedMeshComponent.hpp"
#include "components/MeshComponent.hpp"
#include "graphics/drawing/Batch.hpp"
#include "graphics/drawing/Mesh.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include "graphics/drawing/BatchUnit.hpp"
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
SelfContainedMeshCanvas::SelfContainedMeshCanvas(nanogui::Widget& parent, nanogui::Screen& screen)
: nanogui::Canvas(parent, screen, 1, true, true), mCurrentTime(0), mCamera(mRegistry), mUpdate(true), mShaderManager(ShaderManager(*this)) {
	
	set_background_color(nanogui::Color{70, 130, 180, 255});
	
	
	mSkinnedMeshPreviewShader = ShaderWrapper(mShaderManager->load_shader("skinned_mesh_preview",
																		  "internal/shaders/metal/preview_diffuse_skinned_vs.metal",
																		  "internal/shaders/metal/preview_diffuse_fs.metal",
																		  nanogui::Shader::BlendMode::AlphaBlend));
	
	mMeshPreviewShader = ShaderWrapper(mShaderManager->load_shader("mesh_preview",
																   "internal/shaders/metal/preview_diffuse_vs.metal",
																   "internal/shaders/metal/preview_diffuse_fs.metal",
																   nanogui::Shader::BlendMode::AlphaBlend));
	
	mCamera.add_component<TransformComponent>();
	mCamera.add_component<CameraComponent>(mCamera.get_component<TransformComponent>(),
										   45.0f, 0.01f, 5e3f, 192.0f / 128.0f);
	
	mModelMatrix = glm::identity<glm::mat4>();
	
	// Initialize MeshBatch with its shader
	mMeshBatch = std::make_unique<SelfContainedMeshBatch>(render_pass(), *mMeshPreviewShader);
	
	// Initialize SkinnedMeshBatch with its shader
	mSkinnedMeshBatch = std::make_unique<SelfContainedSkinnedMeshBatch>(render_pass(), *mSkinnedMeshPreviewShader);

}

void SelfContainedMeshCanvas::set_active_actor(std::optional<std::reference_wrapper<Actor>> actor) {
	std::unique_lock<std::mutex> lock(mMutex);
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
				auto& vertices = skinnedData->get_mesh_data().get_vertices();

				// Initialize min and max bounds with extreme values
				mModelMinimumBounds = glm::vec3(std::numeric_limits<float>::max());
				mModelMaximumBounds = glm::vec3(std::numeric_limits<float>::lowest());
				
				for (size_t i = 0; i < vertices.size(); i += 3) {
					auto& vertex = vertices[i];
					
					mModelMinimumBounds = glm::min(mModelMinimumBounds, vertex->get_position());
					mModelMaximumBounds = glm::max(mModelMaximumBounds, vertex->get_position());
				}
				
				auto& skeletonComponent = skinnedData->get_skeleton_component();
				
				size_t numBones = skeletonComponent.get_skeleton().num_bones();
				
				for (size_t i = 0; i < numBones; ++i) {
					Skeleton::Bone& bone = static_cast<Skeleton::Bone&>(skeletonComponent.get_skeleton().get_bone(i));
					
					auto transform = bone.global * glm::inverse(bone.offset);
					
					glm::vec3 scale;
					glm::quat rotation;
					glm::vec3 translation;
					glm::vec3 skew;
					glm::vec4 perspective;
					glm::decompose(transform, scale, rotation, translation, skew, perspective);

					mModelMinimumBounds = glm::min(mModelMinimumBounds, translation);
					mModelMaximumBounds = glm::max(mModelMaximumBounds, translation);
				}
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
					auto& vertices = meshData->get_mesh_data().get_vertices();
					
					// Initialize min and max bounds with extreme values
					mModelMinimumBounds = glm::vec3(std::numeric_limits<float>::max());
					mModelMaximumBounds = glm::vec3(std::numeric_limits<float>::lowest());

					for (size_t i = 0; i < vertices.size(); i += 3) {
						auto& vertex = vertices[i];
						
						mModelMinimumBounds = glm::min(mModelMinimumBounds, vertex->get_position());
						mModelMaximumBounds = glm::max(mModelMaximumBounds, vertex->get_position());
					}
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
	
	if (mPreviewActor.has_value()) {
		auto& camera = mCamera.get_component<CameraComponent>();
		
		// Calculate center and size of the bounding box
		glm::vec3 center = (mModelMinimumBounds + mModelMaximumBounds) * 0.5f;
		glm::vec3 size = mModelMinimumBounds - mModelMaximumBounds;
		float distance = glm::length(size); // Camera distance from object based on size
		
		// Set the camera position and view direction
		auto& cameraTransform = mCamera.get_component<TransformComponent>();
		
		cameraTransform.set_translation(center - glm::vec3(0.0f, 0.0f, -distance));

		camera.look_at(center);
	}
}

void SelfContainedMeshCanvas::draw_content(const nanogui::Matrix4f& view,
										   const nanogui::Matrix4f& projection) {
	
	DrawableComponent& drawableComponent = mPreviewActor->get().get_component<DrawableComponent>();
	
	Drawable& drawableRef = drawableComponent.drawable();
	
	if (mPreviewActor->get().find_component<SkinnedAnimationComponent>()) {
		auto& component = mPreviewActor->get().get_component<SkinnedAnimationComponent>();
		
		component.Unfreeze();
		
		component.evaluate_provider(mCurrentTime, PlaybackModifier::Forward);
		
		mCurrentTime += 1;
	}
	
	drawableRef.draw_content(CanvasUtils::glm_to_nanogui(mModelMatrix), view, projection);
	
	mMeshBatch->draw_content(view, projection);
	
	mSkinnedMeshBatch->draw_content(view, projection);
	
	if (mPreviewActor->get().find_component<SkinnedAnimationComponent>()) {
		auto& animationComponent = mPreviewActor->get().get_component<SkinnedAnimationComponent>();
		
		animationComponent.reset_pose();
		
	}
}

void SelfContainedMeshCanvas::draw_contents() {
	std::unique_lock<std::mutex> lock(mMutex);
	
	if (mPreviewActor.has_value() && mUpdate) {
		update_camera_view();
		
		auto& camera = mCamera.get_component<CameraComponent>();
		
		camera.update_view();
		
		render_pass().clear_color(0, nanogui::Color(0.0f, 0.0f, 0.0f, 1.0f));
		
		render_pass().clear_depth(1.0f);
		
		render_pass().set_depth_test(nanogui::RenderPass::DepthTest::Less, true);
		
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

void SelfContainedMeshCanvas::rotate(const glm::vec3& axis, float angleDegrees) {
	// Normalize the rotation axis to ensure consistent rotation
	glm::vec3 normalizedAxis = glm::normalize(axis);
	
	// Convert angle from degrees to radians as glm::angleAxis expects radians
	float angleRadians = glm::radians(angleDegrees);
	
	// Create a quaternion representing the rotation
	glm::quat rotationQuat = glm::angleAxis(angleRadians, normalizedAxis);
	
	// Convert the quaternion to a rotation matrix
	glm::mat4 rotationMatrix = glm::toMat4(rotationQuat);
	
	// Apply the rotation to the model matrix
	// You can choose the order based on your rotation requirements
	// Pre-multiplying applies the rotation in world space
	// Post-multiplying applies the rotation in local space
	// Here, we'll post-multiply to rotate the model in its local space
	mModelMatrix = mModelMatrix * rotationMatrix;
	
	// Alternatively, using quaternion multiplication:
	// glm::quat currentQuat = glm::quat_cast(mModelMatrix);
	// currentQuat = rotationQuat * currentQuat; // Note the order
	// mModelMatrix = glm::toMat4(currentQuat);
}
