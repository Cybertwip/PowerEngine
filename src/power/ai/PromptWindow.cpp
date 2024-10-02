#include "PromptWindow.hpp"

#include "ShaderManager.hpp"

#include "ai/DeepMotionApiClient.hpp"

#include "filesystem/MeshActorImporter.hpp"
#include "graphics/drawing/BatchUnit.hpp"
#include "graphics/drawing/MeshActorBuilder.hpp"
#include "graphics/drawing/SelfContainedMeshBatch.hpp"
#include "graphics/drawing/SharedSelfContainedMeshCanvas.hpp"
#include "graphics/drawing/SelfContainedSkinnedMeshBatch.hpp"
#include "ui/ResourcesPanel.hpp"

#include <nanogui/button.h>
#include <nanogui/layout.h>
#include <nanogui/screen.h>
#include <nanogui/textbox.h>

#include <sstream>

PromptWindow::PromptWindow(nanogui::Widget* parent, ResourcesPanel& resourcesPanel, DeepMotionApiClient& deepMotionApiClient, nanogui::RenderPass& renderpass, ShaderManager& shaderManager)
: nanogui::Window(parent->screen()), mResourcesPanel(resourcesPanel), mDeepMotionApiClient(deepMotionApiClient) {
	
	set_fixed_size(nanogui::Vector2i(400, 512)); // Adjusted height for additional UI elements
	set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Middle));
	set_title("Import Asset");
	
	// Close Button
	auto close_button = new nanogui::Button(button_panel(), "X");
	close_button->set_fixed_size(nanogui::Vector2i(20, 20));
	close_button->set_callback([this]() {
		this->set_visible(false);
		this->set_modal(false);
		mPreviewCanvas->set_update(false);
	});
	
	// Preview Canvas
	mPreviewCanvas = new SharedSelfContainedMeshCanvas(this);
	mPreviewCanvas->set_fixed_size(nanogui::Vector2i(256, 256));
	mPreviewCanvas->set_aspect_ratio(1.0f);
	
	// Mesh and Skinned Mesh Batches
	mMeshBatch = std::make_unique<SelfContainedMeshBatch>(renderpass, mPreviewCanvas->get_mesh_shader());
	mSkinnedMeshBatch = std::make_unique<SelfContainedSkinnedMeshBatch>(renderpass, mPreviewCanvas->get_skinned_mesh_shader());
	mBatchUnit = std::make_unique<BatchUnit>(*mMeshBatch, *mSkinnedMeshBatch);
	mMeshActorBuilder = std::make_unique<MeshActorBuilder>(*mBatchUnit);
	mMeshActorImporter = std::make_unique<MeshActorImporter>();
	
	// Add Text Box for User Input (e.g., Animation Name)
	auto input_panel = new nanogui::Widget(this);
	input_panel->set_layout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Middle, 10, 10));
	
	auto label = new nanogui::Label(input_panel, "Description:", "sans-bold");
	mInputTextBox = new nanogui::TextBox(input_panel, "");
	mInputTextBox->set_fixed_size(nanogui::Vector2i(200, 96));
	
	mInputTextBox->set_alignment(nanogui::TextBox::Alignment::Left);
	
	mInputTextBox->set_placeholder("Enter the animation generation prompt");
	mInputTextBox->set_font_size(14);
	mInputTextBox->set_editable(true);
	
	// Add Submit Button
	mSubmitButton = new nanogui::Button(this, "Submit");
	mSubmitButton->set_callback([this]() {
		nanogui::async([this]() { this->ImportIntoProject(); });
	});
	mSubmitButton->set_tooltip("Submit the animation import");
	mSubmitButton->set_fixed_width(256);
	
	// Import Button (if still needed)
	// If "Submit" replaces "Import", you can remove or rename this button
	auto importButton = new nanogui::Button(this, "Import");
	importButton->set_callback([this]() {
		nanogui::async([this]() { this->ImportIntoProject(); });
	});
	importButton->set_tooltip("Import the selected asset with the chosen options");
	importButton->set_fixed_width(256);
}

void PromptWindow::Preview(const std::string& path, const std::string& directory) {
	set_visible(true);
	set_modal(true);
	
	
	auto actor = std::make_shared<Actor>(mDummyRegistry);
	
	mMeshActorBuilder->build(*actor, path, mPreviewCanvas->get_mesh_shader(), mPreviewCanvas->get_skinned_mesh_shader());

	if (path.find(".psk") == std::string::npos) {
		// Mesh rigging is still not implemented
		mSubmitButton->set_enabled(false);
	}

	
	mPreviewCanvas->set_active_actor(actor);
	mPreviewCanvas->set_update(true);
}

void PromptWindow::ImportIntoProject() {
	std::string animationName = mInputTextBox->value();
	if (animationName.empty()) {
		// Optionally show an error message to the user
		return;
	}
	
	mPreviewCanvas->take_snapshot([this](std::vector<uint8_t>& pixels) {
		auto& serializer = mCompressedMeshData->mMesh.mSerializer;
		
		// Proceed with serialization
		serializer->write_header_uint64(pixels.size());
		serializer->write_header_raw(pixels.data(), pixels.size()); // Corrected variable name
		
		// Since we're enforcing only animations, ensure only animations are persisted
		mCompressedMeshData->persist_animations(); // Assume a method to persist animations with a name
		
		nanogui::async([this]() {
			mResourcesPanel.refresh_file_view();
		});
		
		mPreviewCanvas->set_active_actor(nullptr);
		mPreviewCanvas->set_update(false);
		
		set_visible(false);
		set_modal(false);
	});
}

void PromptWindow::ProcessEvents() {
	mPreviewCanvas->process_events();
}
