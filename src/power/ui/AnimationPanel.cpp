#include "ui/AnimationPanel.hpp"

#include <nanogui/canvas.h>
#include <nanogui/icons.h>
#include <nanogui/layout.h>
#include <nanogui/renderpass.h>
#include <nanogui/screen.h>
#include <nanogui/toolbutton.h>

#include "ShaderManager.hpp"

#include "actors/Actor.hpp"
#include "components/DrawableComponent.hpp"
#include "components/SkinnedAnimationComponent.hpp"
#include "components/SkinnedMeshComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/CameraComponent.hpp"

#include "graphics/drawing/Batch.hpp"
#include "graphics/drawing/SkinnedMesh.hpp"

#include "graphics/shading/MeshData.hpp"

#include "graphics/shading/ShaderWrapper.hpp"

#include "import/SkinnedFbx.hpp"

class SelfContainedMeshCanvas : public nanogui::Canvas {
public:
	SelfContainedMeshCanvas(Widget* parent) : nanogui::Canvas(parent, 1, true, true), mCurrentTime(0), mCamera(mRegistry), mShaderManager(*this), mSkinnedMeshPreviewShader(*mShaderManager.load_shader("skinned_mesh_preview", "shaders/metal/preview_diffuse_skinned_vs.metal", "shaders/metal/preview_diffuse_fs.metal", nanogui::Shader::BlendMode::None)) {
		set_background_color(nanogui::Color{70, 130, 180, 255});
		
		mCamera.add_component<TransformComponent>();
		mCamera.add_component<CameraComponent>(mCamera.get_component<TransformComponent>(), 45.0f, 0.01f, 5e3f, 192.0f / 128.0f);
	}

	void set_active_actor(std::optional<std::reference_wrapper<Actor>> actor) {
		clear();
		
		mPreviewActor = actor;
		
		if (mPreviewActor.has_value()) {
			if (mPreviewActor->get().find_component<SkinnedAnimationComponent>()) {
				
				DrawableComponent& drawableComponent = mPreviewActor->get().get_component<DrawableComponent>();
				
				const SkinnedMeshComponent& meshComponent = static_cast<const SkinnedMeshComponent&>(drawableComponent.drawable());
				
				for (auto& skinnedData : meshComponent.get_skinned_mesh_data()) {
					add_mesh(*skinnedData);
					append(*skinnedData);
				}
			}
		}
	}
	
private:
	void add_mesh(std::reference_wrapper<SkinnedMesh> mesh) {
		
		auto it = std::find_if(mMeshes.begin(), mMeshes.end(), [mesh](auto kp) {
			return kp.first->identifier() == mesh.get().get_shader().identifier();
		});
		
		if (it != mMeshes.end()) {
			it->second.push_back(mesh);
		} else {
			mMeshes[&(mesh.get().get_shader())].push_back(mesh);
		}
	}

	void append(std::reference_wrapper<SkinnedMesh> meshRef) {
		auto& mesh = meshRef.get();
		
		auto& shader = mSkinnedMeshPreviewShader;
		int identifier = shader.identifier();
		auto& indexer = mVertexIndexingMap[identifier];
		
		// Append vertex data using getters
		mBatchPositions[identifier].insert(mBatchPositions[identifier].end(),
										   mesh.get_flattened_positions().begin(),
										   mesh.get_flattened_positions().end());
		mBatchTexCoords1[shader.identifier()].insert(mBatchTexCoords1[shader.identifier()].end(),
													 mesh.get_flattened_tex_coords1().begin(),
													 mesh.get_flattened_tex_coords1().end());
		mBatchTexCoords2[shader.identifier()].insert(mBatchTexCoords2[shader.identifier()].end(),
													 mesh.get_flattened_tex_coords2().begin(),
													 mesh.get_flattened_tex_coords2().end());
		mBatchTextureIds[shader.identifier()].insert(mBatchTextureIds[shader.identifier()].end(),
													 mesh.get_flattened_texture_ids().begin(),
													 mesh.get_flattened_texture_ids().end());
				
		mBatchBoneIds[shader.identifier()].insert(mBatchBoneIds[shader.identifier()].end(),
												  mesh.get_flattened_bone_ids().begin(),
												  mesh.get_flattened_bone_ids().end());
		
		
		mBatchBoneWeights[shader.identifier()].insert(mBatchBoneWeights[shader.identifier()].end(),
													  mesh.get_flattened_weights().begin(),
													  mesh.get_flattened_weights().end());
		
		// Adjust and append indices
		auto& indices = mesh.get_mesh_data().get_indices()
		;
		for (auto index : indices) {
			mBatchIndices[shader.identifier()].push_back(index + indexer.mVertexOffset);
		}
		
		mMeshStartIndices[shader.identifier()].push_back(indexer.mIndexOffset);
		
		indexer.mIndexOffset += mesh.get_mesh_data().get_indices().size();
		indexer.mVertexOffset += mesh.get_mesh_data().get_skinned_vertices().size();
		
		upload_vertex_data(shader, identifier);
	}
	
	
	void draw_content(const nanogui::Matrix4f& view,
										const nanogui::Matrix4f& projection) {
		
#if defined(NANOGUI_USE_OPENGL) || defined(NANOGUI_USE_GLES)
		// Enable stencil test
		glEnable(GL_STENCIL_TEST);
		glEnable(GL_DEPTH_TEST);
		
		// Clear stencil buffer and depth buffer
		glClear(GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// First pass: Mark the stencil buffer
		glStencilFunc(GL_ALWAYS, 1, 0xFF);  // Always pass stencil test
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);  // Replace stencil buffer with 1 where actors are drawn
		glStencilMask(0xFF);  // Enable writing to the stencil buffer
#endif
		
		for (auto& [_, mesh_vector] : mMeshes) {
			auto& shader = mSkinnedMeshPreviewShader;
			int identifier = shader.identifier();
			
			render_pass()->pop_depth_test_state(identifier);
			
			for (int i = 0; i<mesh_vector.size(); ++i) {
				auto& mesh = mesh_vector[i].get();
				
				auto& shader = mSkinnedMeshPreviewShader;
				
				// Set uniforms and draw the mesh content
				shader.set_uniform("aView", view);
				shader.set_uniform("aProjection", projection);
				
				// Set the model matrix for the current mesh
				shader.set_uniform("aModel", nanogui::Matrix4f::identity());
				
//				if (mReverse) {
//					mCurrentTime += 0.016 * -1;
//				} else {
//					mCurrentTime += 0.016 * 1;
//				}
				int duration = mesh.get_skinned_component().get_animation_duration();
				
				mCurrentTime += 1;

				mCurrentTime = fmax(0, fmod(duration + mCurrentTime, duration));

				// Apply skinning and animations (if any)
				auto bones = mesh.get_skinned_component().get_bones_at_time(mCurrentTime);
				
				shader.set_buffer(
								  "bones",
								  nanogui::VariableType::Float32,
								  {bones.size(), sizeof(SkinnedAnimationComponent::BoneCPU) / sizeof(float)},
								  bones.data(), 11);
				
				// Upload materials for the current mesh
				upload_material_data(shader, mesh.get_mesh_data().get_material_properties());
				
				// Calculate the range of indices to draw for this mesh
				size_t startIdx = mMeshStartIndices[identifier][i];
				size_t count = (i < mesh_vector.size() - 1) ?
				(mMeshStartIndices[identifier][i + 1] - startIdx) :
				(mBatchIndices[identifier].size() - startIdx);
				
				// Begin shader program
				shader.begin();
				
				// Draw the mesh segment
				shader.draw_array(nanogui::Shader::PrimitiveType::Triangle,
								  startIdx, count, true);
				
				// End shader program
				shader.end();
			}
			
		}
	}
	
	void draw_contents() override {
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
				float distance = glm::length(size) * 1.5f; // Camera distance from object based on size
				
				// Set the camera position and view direction
				auto& cameraTransform = mCamera.get_component<TransformComponent>();
				

				center.y = -center.y;
				
				cameraTransform.set_translation(center + glm::vec3(0.0f, 0.0f, distance));
				camera.look_at(mPreviewActor->get());
			}
			
			render_pass()->clear_color(0, nanogui::Color(0.0f, 0.0f, 0.0f, 1.0f));
			
			render_pass()->clear_depth(1.0f);
			
			render_pass()->set_depth_test(nanogui::RenderPass::DepthTest::Less, true);

			draw_content(camera.get_view(),
						 camera.get_projection());
		}
	}
	
	std::optional<std::reference_wrapper<Actor>> mPreviewActor;
	
	entt::registry mRegistry;
	int mCurrentTime;
	Actor mCamera;
	
	ShaderManager mShaderManager;
	ShaderWrapper mSkinnedMeshPreviewShader;
	
private:
	
	void clear() {
		mMeshes.clear();
		mBatchPositions.clear();
		mBatchTexCoords1.clear();
		mBatchTexCoords2.clear();
		mBatchTextureIds.clear();
		mBatchIndices.clear();
		mBatchMaterials.clear();
		mMeshStartIndices.clear();
		mVertexIndexingMap.clear();
	}

	void upload_vertex_data(ShaderWrapper& shader, int identifier) {
		// Upload consolidated data to GPU
		shader.persist_buffer("aPosition", nanogui::VariableType::Float32, {mBatchPositions[identifier].size() / 3, 3},
							  mBatchPositions[identifier].data());
		shader.persist_buffer("aTexcoords1", nanogui::VariableType::Float32, {mBatchTexCoords1[identifier].size() / 2, 2},
							  mBatchTexCoords1[identifier].data());
		shader.persist_buffer("aTexcoords2", nanogui::VariableType::Float32, {mBatchTexCoords2[identifier].size() / 2, 2},
							  mBatchTexCoords2[identifier].data());
		shader.persist_buffer("aTextureId", nanogui::VariableType::Int32, {mBatchTextureIds[identifier].size(), 1},
							  mBatchTextureIds[identifier].data());
		// Set Buffer for Bone IDs
		shader.persist_buffer("aBoneIds", nanogui::VariableType::Int32, {mBatchBoneIds[identifier].size() / 4, 4},
							  mBatchBoneIds[identifier].data());
		
		// Set Buffer for Weights
		shader.persist_buffer("aWeights", nanogui::VariableType::Float32, {mBatchBoneWeights[identifier].size() / 4, 4},
							  mBatchBoneWeights[identifier].data());
		// Upload indices
		shader.persist_buffer("indices", nanogui::VariableType::UInt32, {mBatchIndices[identifier].size()},
							  mBatchIndices[identifier].data());
	}
	
	void upload_material_data(ShaderWrapper& shader, const std::vector<std::shared_ptr<MaterialProperties>>& materialData) {
#if defined(NANOGUI_USE_METAL)
		// Ensure we have a valid number of materials
		size_t numMaterials = materialData.size();
		
		// Create a CPU-side array of Material structs
		std::vector<nanogui::Texture*> texturesCPU(numMaterials);
		std::vector<MaterialCPU> materialsCPU(numMaterials);
		
		for (size_t i = 0; i < numMaterials; ++i) {
			auto& material = *materialData[i];
			MaterialCPU& materialCPU = materialsCPU[i];
			
			// Copy ambient, diffuse, and specular as arrays
			memcpy(&materialCPU.mAmbient[0], glm::value_ptr(material.mAmbient), sizeof(materialCPU.mAmbient));
			
			memcpy(&materialCPU.mDiffuse[0], glm::value_ptr(material.mDiffuse), sizeof(materialCPU.mDiffuse));
			
			memcpy(&materialCPU.mSpecular[0], glm::value_ptr(material.mSpecular), sizeof(materialCPU.mSpecular));
			
			// Copy the rest of the individual fields
			materialCPU.mShininess = material.mShininess;
			materialCPU.mOpacity = material.mOpacity;
			materialCPU.mHasDiffuseTexture = material.mHasDiffuseTexture ? 1.0f : 0.0f;
			
			// Set the diffuse texture (if it exists) or a dummy texture
			std::string textureBaseName = "textures";
			if (material.mHasDiffuseTexture) {
				shader.set_texture(textureBaseName, *material.mTextureDiffuse, i);
			} else {
				shader.set_texture(textureBaseName, Batch::get_dummy_texture(), i);
			}
			
		}
		
		shader.set_buffer(
						  "materials",
						  nanogui::VariableType::Float32,
						  {numMaterials, sizeof(MaterialCPU) / sizeof(float)},
						  materialsCPU.data()
						  );
		
		size_t dummy_texture_count = shader.get_buffer_size("textures");
		
		for (size_t i = numMaterials; i<dummy_texture_count; ++i) {
			std::string textureBaseName = "textures";
			
			shader.set_texture(textureBaseName, Batch::get_dummy_texture(), i);
		}
		
#else
		for (int i = 0; i < materialData.size(); ++i) {
			// Create the uniform name dynamically for each material (e.g., "materials[0].ambient")
			auto& material = *materialData[i];
			std::string baseName = "materials[" + std::to_string(i) + "].";
			
			// Uploading vec3 uniforms for each material (ambient, diffuse, specular)
			mShader.set_uniform(baseName + "ambient",
								nanogui::Vector3f(material.mAmbient.x, material.mAmbient.y, material.mAmbient.z));
			mShader.set_uniform(baseName + "diffuse",
								nanogui::Vector3f(material.mDiffuse.x, material.mDiffuse.y, material.mDiffuse.z));
			mShader.set_uniform(baseName + "specular",
								nanogui::Vector3f(material.mSpecular.x, material.mSpecular.y, material.mSpecular.z));
			
			// Upload float uniforms for shininess and opacity
			mShader.set_uniform(baseName + "shininess", material.mShininess);
			mShader.set_uniform(baseName + "opacity", material.mOpacity);
			
			// Convert boolean to integer for setting in the shader (0 or 1)
			mShader.set_uniform(baseName + "diffuse_texture", material.mHasDiffuseTexture ? 1.0f : 0.0f);
			
			// Set the diffuse texture (if it exists) or a dummy texture
			std::string textureBaseName = "textures[" + std::to_string(i) + "]";
			if (material.mHasDiffuseTexture) {
				mShader.set_texture(textureBaseName, material.mTextureDiffuse.get());
			} else {
				mShader.set_texture(textureBaseName, mDummyTexture.get());
			}
		}
#endif
	}
	
	struct VertexIndexer {
		size_t mVertexOffset = 0;
		size_t mIndexOffset = 0;
	};
		
	std::unordered_map<ShaderWrapper*, std::vector<std::reference_wrapper<SkinnedMesh>>> mMeshes;
	
	// Consolidated buffers
	std::unordered_map<int, std::vector<float>> mBatchPositions;
	std::unordered_map<int, std::vector<float>> mBatchTexCoords1;
	std::unordered_map<int, std::vector<float>> mBatchTexCoords2;
	std::unordered_map<int, std::vector<int>> mBatchTextureIds;
	std::unordered_map<int, std::vector<int>> mBatchBoneIds;
	std::unordered_map<int, std::vector<float>> mBatchBoneWeights;
	std::unordered_map<int, std::vector<unsigned int>> mBatchIndices;
	std::vector<std::shared_ptr<MaterialProperties>> mBatchMaterials;
	
	// Offset tracking
	std::unordered_map<int, std::vector<size_t>> mMeshStartIndices;
	
	std::unordered_map<int, VertexIndexer> mVertexIndexingMap;
};

AnimationPanel::AnimationPanel(nanogui::Widget &parent)
: Panel(parent, "Animation"), mActiveActor(std::nullopt) {
	set_position(nanogui::Vector2i(0, 0));
	set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 10, 10));
	
	auto *playbackLayout = new nanogui::GridLayout(nanogui::Orientation::Horizontal, 2,
												   nanogui::Alignment::Middle, 0, 0);
	
	Widget *playbackPanel = new Widget(this);
	
	
	playbackLayout->set_row_alignment(nanogui::Alignment::Fill);
	
	playbackPanel->set_layout(playbackLayout);
	
	Widget *canvasPanel = new Widget(this);
	canvasPanel->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 0, 0));
	
	mReversePlayButton = new nanogui::ToolButton(playbackPanel, FA_BACKWARD);
	mReversePlayButton->set_tooltip("Reverse");
	
	mReversePlayButton->set_change_callback([this](bool active){
		if (mActiveActor.has_value()) {
			auto& animation = mActiveActor->get().get_component<SkinnedAnimationComponent>();
			
			animation.set_reverse(true);
			animation.set_playing(active);
		}
	});
	
	
	mPlayPauseButton = new nanogui::ToolButton(playbackPanel, FA_FORWARD);
	mPlayPauseButton->set_tooltip("Play");
	
	mPlayPauseButton->set_change_callback([this](bool active){
		if (mActiveActor.has_value()) {
			auto& animation = mActiveActor->get().get_component<SkinnedAnimationComponent>();
			
			animation.set_reverse(false);
			animation.set_playing(active);
		}
	});
	
	mPreviewCanvas = new SelfContainedMeshCanvas(canvasPanel);
	
	set_active_actor(std::nullopt);
}

AnimationPanel::~AnimationPanel() {
	
}

void AnimationPanel::parse_file(const std::string& path) {
	if (mActiveActor.has_value()) {
		auto model = std::make_unique<SkinnedFbx>(path);
		
		model->LoadModel();
		model->TryImportAnimations();
		
		std::unique_ptr<Drawable> drawableComponent;
		
		if (model->GetSkeleton() != nullptr) {
			std::vector<std::unique_ptr<SkinnedMesh>> skinnedMeshComponentData;
			
			auto pdo = std::make_unique<SkinnedAnimationComponent::SkinnedAnimationPdo> (std::move(model->GetSkeleton()));
			
			for (auto& animation : model->GetAnimationData()) {
				pdo->mAnimationData.push_back(std::move(animation));
			}
			
			auto& skinnedComponent = mActiveActor->get().get_component<SkinnedAnimationComponent>();
			
			
			skinnedComponent.set_pdo(std::move(pdo));
			
			mPlayPauseButton->set_pushed(false);
			mReversePlayButton->set_pushed(false);
		}
	}
}

void AnimationPanel::set_active_actor(std::optional<std::reference_wrapper<Actor>> actor) {
	
	mActiveActor = actor;
	
	
	if (mActiveActor.has_value()) {
		if (mActiveActor->get().find_component<SkinnedAnimationComponent>()) {
			set_visible(true);
			parent()->perform_layout(screen()->nvg_context());
			mPlayPauseButton->set_pushed(false);
			mReversePlayButton->set_pushed(false);
			
		} else {
			set_visible(false);
			parent()->perform_layout(screen()->nvg_context());
		}
	} else {
		set_visible(false);
		parent()->perform_layout(screen()->nvg_context());
	}
	
	mPreviewCanvas->set_active_actor(mActiveActor);
}
