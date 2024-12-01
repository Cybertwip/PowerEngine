#include "ImportWindow.hpp"

#include "ShaderManager.hpp"

#include "filesystem/MeshActorImporter.hpp"
#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "graphics/drawing/SelfContainedMeshBatch.hpp"
#include "graphics/drawing/SharedSelfContainedMeshCanvas.hpp"

#include "graphics/drawing/SelfContainedSkinnedMeshBatch.hpp"

#include "ui/ResourcesPanel.hpp"

#include <nanogui/button.h>
#include <nanogui/checkbox.h>
#include <nanogui/layout.h>
#include <nanogui/screen.h>

#include <openssl/md5.h>

#include <sstream>

ImportWindow::ImportWindow(nanogui::Screen& parent, ResourcesPanel& resourcesPanel, nanogui::RenderPass& renderpass, ShaderManager& shaderManager) : nanogui::Window(parent), mResourcesPanel(resourcesPanel), mDummyAnimationTimeProvider(60 * 30), mRenderPass(renderpass) {
	
	set_fixed_size(nanogui::Vector2i(400, 320));
	set_layout(std::make_unique<nanogui::GroupLayout>());
	set_title("Import Asset");
	
	// Close Button
	mCloseButton = std::make_shared<nanogui::Button>(button_panel(), "X");
	mCloseButton->set_fixed_size(nanogui::Vector2i(20, 20));
	mCloseButton->set_callback([this]() {
		mPreviewCanvas->set_update(false);
		
		mPreviewCanvas->set_active_actor(nullptr);
		
		this->set_visible(false);
		this->set_modal(false);
	});
	
	mPreviewCanvas = std::make_shared<SharedSelfContainedMeshCanvas>(*this, parent);
	
	mMeshBatch = std::make_unique<SelfContainedMeshBatch>(mRenderPass, mPreviewCanvas->get_mesh_shader());
	
	mSkinnedMeshBatch = std::make_unique<SelfContainedSkinnedMeshBatch>(mRenderPass, mPreviewCanvas->get_skinned_mesh_shader());
	
	mBatchUnit = std::make_unique<BatchUnit>(*mMeshBatch, *mSkinnedMeshBatch);
	
	mMeshActorBuilder = std::make_unique<MeshActorBuilder>(*mBatchUnit);
	
	
	mPreviewCanvas->set_fixed_size(nanogui::Vector2i(256, 256));
	
	mPreviewCanvas->set_aspect_ratio(1.0f);
	
	mMeshActorImporter = std::make_unique<MeshActorImporter>();
	
	// Create a panel or container for checkboxes using GridLayout
	mCheckboxPanel = std::make_shared<nanogui::Widget>(std::make_optional<std::reference_wrapper<nanogui::Widget>>(*this));
	mCheckboxPanel->set_layout(std::make_unique<nanogui::GridLayout>(
																	 nanogui::Orientation::Horizontal, // Layout orientation
																	 2,                               // Number of columns
																	 nanogui::Alignment::Minimum,      // Alignment within cells
																	 0,                              // Column padding
																	 0                               // Row padding
																	 ));
	
	
	// Create "Mesh" checkbox
	mMeshCheckbox = std::make_shared<nanogui::CheckBox>(*mCheckboxPanel, "Mesh");
	mMeshCheckbox->set_checked(true); // Default state as needed
	
	// Create "Animations" checkbox
	mAnimationsCheckbox = std::make_shared<nanogui::CheckBox>(*mCheckboxPanel, "Animations");
	mAnimationsCheckbox->set_checked(true); // Default state as needed
	
	// Create "Import" button
	mImportButton = std::make_shared<nanogui::Button>(*this, "Import");
	mImportButton->set_callback([this]() {
		nanogui::async([this](){this->ImportIntoProject();});
	});
	mImportButton->set_tooltip("Import the selected asset with the chosen options");
	
	mImportButton->set_fixed_width(256);
}

void ImportWindow::Preview(const std::string& path, const std::string& directory) {
	set_visible(true);
	set_modal(true);
	
	auto meshData = mMeshActorImporter->process(path, directory);
		
	auto actor = std::make_shared<Actor>(mDummyRegistry);
	
	std::stringstream compressedData;
	
	meshData->mMesh.mSerializer->get_compressed_data(compressedData);
	
	CompressedSerialization::Deserializer deserializer;
	
	deserializer.initialize(compressedData);
	
	if (meshData->mMesh.mPrecomputedPath.find(".psk") != std::string::npos) {
		
		mMeshActorBuilder->build_skinned(*actor, mDummyAnimationTimeProvider, mDummyAnimationTimeProvider, "DummyActor", deserializer, mPreviewCanvas->get_mesh_shader(), mPreviewCanvas->get_skinned_mesh_shader());
	} else {
		mMeshActorBuilder->build_mesh(*actor, mDummyAnimationTimeProvider, "DummyActor", deserializer, mPreviewCanvas->get_mesh_shader(), mPreviewCanvas->get_skinned_mesh_shader());
	}
	
	mPreviewCanvas->set_active_actor(actor);
	mPreviewCanvas->set_update(true);
	
	if (!meshData->mAnimations.has_value()) {
		mAnimationsCheckbox->set_enabled(false);
		mAnimationsCheckbox->set_checked(false); // Optionally uncheck if disabled
	} else {
		mAnimationsCheckbox->set_enabled(true);
		// Optionally set the checked state based on your requirements
	}
	
	mCompressedMeshData = std::move(meshData);

}

void ImportWindow::ImportIntoProject() {
	mPreviewCanvas->take_snapshot([this](std::vector<uint8_t>& pixels) {
		auto& serializer = mCompressedMeshData->mMesh.mSerializer;
		
		std::stringstream compressedData;
		
		serializer->get_compressed_data(compressedData);

		// Generate the unique hash identifier from the compressed data
		
		uint64_t hash_id[] = { 0, 0 };
		
		Md5::generate_md5_from_compressed_data(compressedData, hash_id);
		
		// Write the unique hash identifier to the header
		serializer->write_header_raw(hash_id, sizeof(hash_id));

		// Proceed with serialization
		serializer->write_header_uint64(pixels.size());
		
		serializer->write_header_raw(pixels.data(), pixels.size());
		
		if (mMeshCheckbox->checked()) {
			mCompressedMeshData->persist_mesh();
		}
		
		if (mAnimationsCheckbox->checked()) {
			
			for (auto& data : *mCompressedMeshData->mAnimations) {
				auto& serializer = data.mSerializer;
				std::stringstream compressedData;
				
				serializer->get_compressed_data(compressedData);

				uint64_t hash_id[] = { 0, 0 };
				
				Md5::generate_md5_from_compressed_data(compressedData, hash_id);
				
				// Write the unique hash identifier to the header
				serializer->write_header_raw(hash_id, sizeof(hash_id));
				
				serializer->write_header_uint64(0);

			}
			
			mCompressedMeshData->persist_animations();
		}

		nanogui::async([this](){
			mResourcesPanel.refresh_file_view();
		});
		
		mPreviewCanvas->set_update(false);
		mPreviewCanvas->set_active_actor(nullptr);
		
		set_visible(false);
		set_modal(false);

	});
	
}

void ImportWindow::ProcessEvents() {
	mDummyAnimationTimeProvider.Update();
	
	mPreviewCanvas->process_events();
}
