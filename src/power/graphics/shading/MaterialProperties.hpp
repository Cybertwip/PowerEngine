#pragma once

#include <glm/glm.hpp>

struct MaterialProperties
{
	glm::vec3 mAmbient{0.8f, 0.8f, 0.8f};
	glm::vec3 mDiffuse{1.0f, 1.0f, 1.0f};
	glm::vec3 mSpecular{0.9f, 0.9f, 0.9f};
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
};
