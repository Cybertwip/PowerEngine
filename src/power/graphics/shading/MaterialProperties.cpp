#include "MaterialProperties.hpp"


// MaterialProperties move constructor
MaterialProperties::MaterialProperties(MaterialProperties&& other) noexcept
: mAmbient(std::move(other.mAmbient)),
mDiffuse(std::move(other.mDiffuse)),
mSpecular(std::move(other.mSpecular)),
mShininess(other.mShininess),
mOpacity(other.mOpacity),
mHasDiffuseTexture(other.mHasDiffuseTexture),
mTextureDiffuse(std::move(other.mTextureDiffuse))
{
	other.mShininess = 0.0f;
	other.mOpacity = 0.0f;
	other.mHasDiffuseTexture = false;
}

// MaterialProperties move assignment operator
MaterialProperties& MaterialProperties::operator=(MaterialProperties&& other) noexcept
{
	if (this != &other) {
		mAmbient = std::move(other.mAmbient);
		mDiffuse = std::move(other.mDiffuse);
		mSpecular = std::move(other.mSpecular);
		mShininess = other.mShininess;
		mOpacity = other.mOpacity;
		mHasDiffuseTexture = other.mHasDiffuseTexture;
		mTextureDiffuse = std::move(other.mTextureDiffuse);
		
		other.mShininess = 0.0f;
		other.mOpacity = 0.0f;
		other.mHasDiffuseTexture = false;
	}
	return *this;
}

// Download texture data from GPU to CPU
std::vector<uint8_t> MaterialProperties::downloadTexture()
{
	if (mHasDiffuseTexture && mTextureDiffuse) {
		size_t textureSize = mTextureDiffuse->size().x() * mTextureDiffuse->size().y() * mTextureDiffuse->bytes_per_pixel();
		std::vector<uint8_t> textureData(textureSize);
		mTextureDiffuse->download(textureData.data());
		return textureData;
	}
	return {};
}
