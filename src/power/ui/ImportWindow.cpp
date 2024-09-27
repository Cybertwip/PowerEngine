#include "ImportWindow.hpp"

#include "ShaderManager.hpp"

#include "filesystem/MeshActorImporter.hpp"
#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "graphics/drawing/MeshBatch.hpp"
#include "graphics/drawing/SharedSelfContainedMeshCanvas.hpp"

#include "graphics/drawing/SkinnedMeshBatch.hpp"

#include "ui/ResourcesPanel.hpp"

#include <nanogui/button.h>
#include <nanogui/checkbox.h>
#include <nanogui/layout.h>
#include <nanogui/screen.h>

ImportWindow::ImportWindow(nanogui::Widget* parent, ResourcesPanel& resourcesPanel, nanogui::RenderPass& renderpass, ShaderManager& shaderManager) : nanogui::Window(parent->screen()), mResourcesPanel(resourcesPanel) {
	
	set_fixed_size(nanogui::Vector2i(400, 320));
	set_layout(new nanogui::GroupLayout());
	set_title("Import Asset");
	
	// Close Button
	auto close_button = new nanogui::Button(button_panel(), "X");
	close_button->set_fixed_size(nanogui::Vector2i(20, 20));
	close_button->set_callback([this]() {
		this->set_visible(false);
		this->set_modal(false);
	});
	
	mMeshBatch = std::make_unique<MeshBatch>(renderpass);
	
	mSkinnedMeshBatch = std::make_unique<SkinnedMeshBatch>(renderpass);
	
	mBatchUnit = std::make_unique<BatchUnit>(*mMeshBatch, *mSkinnedMeshBatch);

	mMeshActorBuilder = std::make_unique<MeshActorBuilder>(*mBatchUnit);
	
	mPreviewCanvas = new SharedSelfContainedMeshCanvas(this);
	
	mPreviewCanvas->set_fixed_size(nanogui::Vector2i(256, 256));
	
	
	mMeshActorImporter = std::make_unique<MeshActorImporter>();
	
	mMeshShader = std::make_unique<ShaderWrapper>(*shaderManager.get_shader("mesh"));
	
	mSkinnedShader = std::make_unique<ShaderWrapper>(*shaderManager.get_shader("skinned_mesh"));

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
		this->ImportIntoProject();
	});
	importButton->set_tooltip("Import the selected asset with the chosen options");
	
	importButton->set_fixed_width(256);
}

void ImportWindow::Preview(const std::string& path, const std::string& directory) {
	set_visible(true);
	set_modal(true);
	
	mCompressedMeshData = std::move(mMeshActorImporter->process(path, directory));
	
	auto actor = std::make_shared<Actor>(mDummyRegistry);
	
	std::vector<char> compressedData;
	
	mCompressedMeshData->mMesh.mSerializer->get_compressed_data(compressedData);
	
	CompressedSerialization::Deserializer deserializer;
	
	deserializer.initialize(compressedData);
	
	if (mCompressedMeshData->mMesh.mPrecomputedPath.find(".psk") != std::string::npos) {
		
		mMeshActorBuilder->build_skinned(*actor, "DummyActor", deserializer, *mMeshShader, *mSkinnedShader);
	} else {
		mMeshActorBuilder->build_mesh(*actor, "DummyActor", deserializer, *mMeshShader, *mSkinnedShader);
	}
	
	mPreviewCanvas->set_active_actor(actor);
	
	if (!mCompressedMeshData->mAnimations.has_value()) {
		mAnimationsCheckbox->set_enabled(false);
		mAnimationsCheckbox->set_checked(false); // Optionally uncheck if disabled
	} else {
		mAnimationsCheckbox->set_enabled(true);
		// Optionally set the checked state based on your requirements
	}
}

void ImportWindow::ImportIntoProject() {
	set_visible(false);
	set_modal(false);

	mCompressedMeshData->persist(mAnimationsCheckbox->checked());
	
	mResourcesPanel.refresh_file_view();
	
//	auto actor = std::make_shared<Actor>(mDummyRegistry);
	
//	mMeshActorBuilder->build(*actor, child->FullPath, *mMeshShader, *mSkinnedShader);
	
//	mOffscreenRenderer->take_snapshot(actor, [this, icon](std::vector<uint8_t> pixels){
//		write_to_png(pixels, 128, 128, 4, "output_image.png");
		
//		return;
		//
		//		auto imageView = new nanogui::ImageView(icon);
		//
		//		imageView->set_size(icon->fixed_size());
		//
		//		imageView->set_fixed_size(icon->fixed_size());
		//
		//		imageView->set_image(new nanogui::Texture(
		//												  pixels.data(),
		//												  pixels.size(),
		//												  nanogui::Texture::InterpolationMode::Bilinear,
		//												  nanogui::Texture::InterpolationMode::Nearest,
		//												  nanogui::Texture::WrapMode::Repeat,
		//												  128,
		//												  128));
		
		//icon->remove_child(mOffscreenRenderer);
		//						mOffscreenRenderer->set_screen(screen()); // prevent crashing
//	});
	
	//imageView->set_visible(true);

}
