#include "Batch.hpp"

#include <nanogui/texture.h>

void Batch::init_dummy_texture() {
	mDummyTexture = std::make_shared<nanogui::Texture>(
													   nanogui::Texture::PixelFormat::RGBA,       // Set pixel format to RGBA
													   nanogui::Texture::ComponentFormat::UInt8,  // Use unsigned 8-bit components for each channel
													   nanogui::Vector2i(1, 1));                              // Size of the texture (1x1)
	
}

std::shared_ptr<nanogui::Texture> Batch::mDummyTexture;
