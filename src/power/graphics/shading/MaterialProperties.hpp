#pragma once

#include "filesystem/CompressedSerialization.hpp"

#include <nanogui/texture.h>

#include <glm/glm.hpp>

#include <functional> // For std::hash

struct SerializableMaterialProperties
{
	uint64_t mIdentifier;
	
	glm::vec4 mAmbient{0.8f, 0.8f, 0.8f, 1.0f};
	glm::vec4 mDiffuse{1.0f, 1.0f, 1.0f, 1.0f};
	glm::vec4 mSpecular{0.9f, 0.9f, 0.9f, 1.0f};
	float mShininess{1.0f};
	float mOpacity{1.0f};
	bool mHasDiffuseTexture{false};
	std::vector<uint8_t> mTextureDiffuse;
	
	// Move constructor
	SerializableMaterialProperties(SerializableMaterialProperties&& other) noexcept
	: mAmbient(std::move(other.mAmbient)),
	mDiffuse(std::move(other.mDiffuse)),
	mSpecular(std::move(other.mSpecular)),
	mShininess(other.mShininess),
	mOpacity(other.mOpacity),
	mHasDiffuseTexture(other.mHasDiffuseTexture),
	mTextureDiffuse(std::move(other.mTextureDiffuse)) // Moves the vector of unique_ptrs
	{
		// Reset other state if needed (optional)
		other.mShininess = 0.0f;
		other.mOpacity = 0.0f;
		other.mHasDiffuseTexture = false;
	}
	
	// Move assignment operator
	SerializableMaterialProperties& operator=(SerializableMaterialProperties&& other) noexcept
	{
		if (this != &other) {
			mAmbient = std::move(other.mAmbient);
			mDiffuse = std::move(other.mDiffuse);
			mSpecular = std::move(other.mSpecular);
			mShininess = other.mShininess;
			mOpacity = other.mOpacity;
			mHasDiffuseTexture = other.mHasDiffuseTexture;
			mTextureDiffuse = std::move(other.mTextureDiffuse);  // Move the vector of unique_ptrs
			
			// Reset other state if needed (optional)
			other.mShininess = 0.0f;
			other.mOpacity = 0.0f;
			other.mHasDiffuseTexture = false;
		}
		return *this;
	}
	
	// Default constructor, copy constructor, etc., as needed
	SerializableMaterialProperties() = default;
	SerializableMaterialProperties(const SerializableMaterialProperties&) = delete; // Disable copy constructor
	SerializableMaterialProperties& operator=(const SerializableMaterialProperties&) = delete; // Disable copy assignment
	
	// Serialize method
	void serialize(CompressedSerialization::Serializer& serializer) const {
		serializer.write_uint32(static_cast<uint32_t>(mIdentifier));
		serializer.write_vec4(mAmbient);
		serializer.write_vec4(mDiffuse);
		serializer.write_vec4(mSpecular);
		serializer.write_float(mShininess);
		serializer.write_float(mOpacity);
		serializer.write_bool(mHasDiffuseTexture);
		// Serialize mTextureDiffuse if needed
		
		if (mHasDiffuseTexture) {
			uint64_t textureSize = static_cast<uint64_t>(mTextureDiffuse.size());
			serializer.write_uint64(textureSize);
			serializer.write_raw(mTextureDiffuse.data(), textureSize);
		}
	}
	
	// Deserialize method
	bool deserialize(CompressedSerialization::Deserializer& deserializer) {
		if (!deserializer.read_uint32(reinterpret_cast<uint32_t&>(mIdentifier))) return false;
		if (!deserializer.read_vec4(mAmbient)) return false;
		if (!deserializer.read_vec4(mDiffuse)) return false;
		if (!deserializer.read_vec4(mSpecular)) return false;
		if (!deserializer.read_float(mShininess)) return false;
		if (!deserializer.read_float(mOpacity)) return false;
		if (!deserializer.read_bool(mHasDiffuseTexture)) return false;
		
		if (mHasDiffuseTexture) {
			uint64_t textureSize = 0;
			if (!deserializer.read_uint64(textureSize)) return false;
			mTextureDiffuse.resize(textureSize);
			if (!deserializer.read_raw(mTextureDiffuse.data(), textureSize)) return false;
		} else {
			mTextureDiffuse.clear();
		}

		// Deserialize mTextureDiffuse if needed
		return true;
	}
};


struct MaterialProperties
{
	uint64_t mIdentifier;
	
	glm::vec4 mAmbient{0.8f, 0.8f, 0.8f, 1.0f};
	glm::vec4 mDiffuse{1.0f, 1.0f, 1.0f, 1.0f};
	glm::vec4 mSpecular{0.9f, 0.9f, 0.9f, 1.0f};
	float mShininess{1.0f};
	float mOpacity{1.0f};
	bool mHasDiffuseTexture{false};
	std::unique_ptr<nanogui::Texture> mTextureDiffuse;  // Movable, not copyable
	
	// Move constructor
	MaterialProperties(MaterialProperties&& other) noexcept
	: mAmbient(std::move(other.mAmbient)),
	mDiffuse(std::move(other.mDiffuse)),
	mSpecular(std::move(other.mSpecular)),
	mShininess(other.mShininess),
	mOpacity(other.mOpacity),
	mHasDiffuseTexture(other.mHasDiffuseTexture),
	mTextureDiffuse(std::move(other.mTextureDiffuse)) // Moves the vector of unique_ptrs
	{
		// Reset other state if needed (optional)
		other.mShininess = 0.0f;
		other.mOpacity = 0.0f;
		other.mHasDiffuseTexture = false;
	}
	
	// Move assignment operator
	MaterialProperties& operator=(MaterialProperties&& other) noexcept
	{
		if (this != &other) {
			mAmbient = std::move(other.mAmbient);
			mDiffuse = std::move(other.mDiffuse);
			mSpecular = std::move(other.mSpecular);
			mShininess = other.mShininess;
			mOpacity = other.mOpacity;
			mHasDiffuseTexture = other.mHasDiffuseTexture;
			mTextureDiffuse = std::move(other.mTextureDiffuse);  // Move the vector of unique_ptrs
			
			// Reset other state if needed (optional)
			other.mShininess = 0.0f;
			other.mOpacity = 0.0f;
			other.mHasDiffuseTexture = false;
		}
		return *this;
	}
	
	// Default constructor, copy constructor, etc., as needed
	MaterialProperties() = default;
	MaterialProperties(const MaterialProperties&) = delete; // Disable copy constructor
	MaterialProperties& operator=(const MaterialProperties&) = delete; // Disable copy assignment
	
	bool deserialize(const SerializableMaterialProperties& properties) {
		mIdentifier = properties.mIdentifier;
		mAmbient = properties.mAmbient;
		mDiffuse = properties.mDiffuse;
		mSpecular = properties.mSpecular;
		mShininess = properties.mShininess;
		mOpacity = properties.mOpacity;
		mHasDiffuseTexture = properties.mHasDiffuseTexture;
		
		if (mHasDiffuseTexture) {
			if (properties.mTextureDiffuse.empty()) {
				std::cerr << "mHasDiffuseTexture is true, but mTextureDiffuse is empty.\n";
				return false;
			}
			
			// Assuming nanogui::Texture can be constructed from raw byte data.
			// Adjust the constructor as per the actual nanogui::Texture implementation.
			try {
				// Example: Creating texture from PNG data
				// nanogui::Texture may require width, height, and format, which need to be determined.
				// For demonstration, we'll assume a constructor that takes raw data and its size.
				// Replace this with actual texture loading logic as per nanogui's API.
				mTextureDiffuse = std::make_unique<nanogui::Texture>(properties.mTextureDiffuse.data(), properties.mTextureDiffuse.size());
			}
			catch (const std::exception& e) {
				std::cerr << "Failed to create nanogui::Texture: " << e.what() << "\n";
				mTextureDiffuse = nullptr;
				return false;
			}
		} else {
			mTextureDiffuse.reset();
		}
		
		return true;
	}
};



#if defined(NANOGUI_USE_METAL)

struct MaterialCPU
{
	float mAmbient[4] = {0.8f, 0.8f, 0.8f, 1.0f};
	float mDiffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float mSpecular[4] = {0.9f, 0.9f, 0.9f, 1.0f};
	float mShininess{1.0f};
	float mOpacity{1.0f};
	float mHasDiffuseTexture{0.0f};
	float _1 = 0.0f;
};

#endif
