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

ImportWindow::ImportWindow(nanogui::Widget* parent, ResourcesPanel& resourcesPanel, nanogui::RenderPass& renderpass, ShaderManager& shaderManager) : nanogui::Window(parent->screen()), mResourcesPanel(resourcesPanel), mDummyAnimationTimeProvider(60 * 30) {
	
	set_fixed_size(nanogui::Vector2i(400, 320));
	set_layout(new nanogui::GroupLayout());
	set_title("Import Asset");
	
	// Close Button
	auto close_button = new nanogui::Button(button_panel(), "X");
	close_button->set_fixed_size(nanogui::Vector2i(20, 20));
	close_button->set_callback([this]() {
		this->set_visible(false);
		this->set_modal(false);
		mPreviewCanvas->set_update(false);
	});
	
	mPreviewCanvas = new SharedSelfContainedMeshCanvas(this);

	mMeshBatch = std::make_unique<SelfContainedMeshBatch>(renderpass, mPreviewCanvas->get_mesh_shader());
	
	mSkinnedMeshBatch = std::make_unique<SelfContainedSkinnedMeshBatch>(renderpass, mPreviewCanvas->get_skinned_mesh_shader());
	
	mBatchUnit = std::make_unique<BatchUnit>(*mMeshBatch, *mSkinnedMeshBatch);

	mMeshActorBuilder = std::make_unique<MeshActorBuilder>(*mBatchUnit);
	
	
	mPreviewCanvas->set_fixed_size(nanogui::Vector2i(256, 256));
	
	mPreviewCanvas->set_aspect_ratio(1.0f);
	
	mMeshActorImporter = std::make_unique<MeshActorImporter>();
	
	// Create a panel or container for checkboxes using GridLayout
	auto checkbox_panel = new nanogui::Widget(this);
	checkbox_panel->set_layout(new nanogui::GridLayout(
													   nanogui::Orientation::Horizontal, // Layout orientation
													   2,                               // Number of columns
													   nanogui::Alignment::Minimum,      // Alignment within cells
													   0,                              // Column padding
													   0                               // Row padding
													   ));
	

	// Create "Mesh" checkbox
	mMeshCheckbox = new nanogui::CheckBox(checkbox_panel, "Mesh");
	mMeshCheckbox->set_checked(true); // Default state as needed
	
	// Create "Animations" checkbox
	mAnimationsCheckbox = new nanogui::CheckBox(checkbox_panel, "Animations");
	mAnimationsCheckbox->set_checked(true); // Default state as needed

	
	// Create "Import" button
	auto importButton = new nanogui::Button(this, "Import");
	importButton->set_callback([this]() {
		nanogui::async([this](){this->ImportIntoProject();});
	});
	importButton->set_tooltip("Import the selected asset with the chosen options");
	
	importButton->set_fixed_width(256);
}

void ImportWindow::Preview(const std::string& path, const std::string& directory) {
	set_visible(true);
	set_modal(true);
	
	mCompressedMeshData = std::move(mMeshActorImporter->process(path, directory));
	
	auto actor = std::make_shared<Actor>(mDummyRegistry);
	
	std::stringstream compressedData;
	
	mCompressedMeshData->mMesh.mSerializer->get_compressed_data(compressedData);
	
	CompressedSerialization::Deserializer deserializer;
	
	deserializer.initialize(compressedData);
	
	if (mCompressedMeshData->mMesh.mPrecomputedPath.find(".psk") != std::string::npos) {
		
		mMeshActorBuilder->build_skinned(*actor, mDummyAnimationTimeProvider, "DummyActor", deserializer, mPreviewCanvas->get_mesh_shader(), mPreviewCanvas->get_skinned_mesh_shader());
	} else {
		mMeshActorBuilder->build_mesh(*actor, mDummyAnimationTimeProvider, "DummyActor", deserializer, mPreviewCanvas->get_mesh_shader(), mPreviewCanvas->get_skinned_mesh_shader());
	}
	
	mPreviewCanvas->set_active_actor(actor);
	mPreviewCanvas->set_update(true);
	
	if (!mCompressedMeshData->mAnimations.has_value()) {
		mAnimationsCheckbox->set_enabled(false);
		mAnimationsCheckbox->set_checked(false); // Optionally uncheck if disabled
	} else {
		mAnimationsCheckbox->set_enabled(true);
		// Optionally set the checked state based on your requirements
	}
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
		
		mCompressedMeshData->persist(mAnimationsCheckbox->checked());

		nanogui::async([this](){
			mResourcesPanel.refresh_file_view();
		});
		mPreviewCanvas->set_active_actor(nullptr);
		mPreviewCanvas->set_update(false);
		
		set_visible(false);
		set_modal(false);

	});
	
}

void ImportWindow::ProcessEvents() {
	mDummyAnimationTimeProvider.Update();
	
	mPreviewCanvas->process_events();
}
