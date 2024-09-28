#include "MaterialProperties.hpp"

// SerializableMaterialProperties move constructor
SerializableMaterialProperties::SerializableMaterialProperties(SerializableMaterialProperties&& other) noexcept
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

// SerializableMaterialProperties move assignment operator
SerializableMaterialProperties& SerializableMaterialProperties::operator=(SerializableMaterialProperties&& other) noexcept
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

// Constructor that converts MaterialProperties to SerializableMaterialProperties
SerializableMaterialProperties::SerializableMaterialProperties(MaterialProperties& material)
{
	mIdentifier = material.mIdentifier;
	mAmbient = material.mAmbient;
	mDiffuse = material.mDiffuse;
	mSpecular = material.mSpecular;
	mShininess = material.mShininess;
	mOpacity = material.mOpacity;
	mHasDiffuseTexture = material.mHasDiffuseTexture;
	
	if (mHasDiffuseTexture && material.mTextureDiffuse) {
		mTextureDiffuse = material.downloadTexture();
	}
}

// Serialize function
void SerializableMaterialProperties::serialize(CompressedSerialization::Serializer& serializer) const
{
	serializer.write_uint32(static_cast<uint32_t>(mIdentifier));
	serializer.write_vec4(mAmbient);
	serializer.write_vec4(mDiffuse);
	serializer.write_vec4(mSpecular);
	serializer.write_float(mShininess);
	serializer.write_float(mOpacity);
	serializer.write_bool(mHasDiffuseTexture);
	
	if (mHasDiffuseTexture) {
		uint64_t textureSize = static_cast<uint64_t>(mTextureDiffuse.size());
		serializer.write_uint64(textureSize);
		serializer.write_raw(mTextureDiffuse.data(), textureSize);
	}
}

// Deserialize function
bool SerializableMaterialProperties::deserialize(CompressedSerialization::Deserializer& deserializer)
{
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
	
	return true;
}

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

// MaterialProperties deserialize method
bool MaterialProperties::deserialize(const SerializableMaterialProperties& properties)
{
	mIdentifier = properties.mIdentifier;
	mAmbient = properties.mAmbient;
	mDiffuse = properties.mDiffuse;
	mSpecular = properties.mSpecular;
	mShininess = properties.mShininess;
	mOpacity = properties.mOpacity;
	mHasDiffuseTexture = properties.mHasDiffuseTexture;
	
	if (mHasDiffuseTexture && !properties.mTextureDiffuse.empty()) {
		try {
			mTextureDiffuse = std::make_unique<nanogui::Texture>(properties.mTextureDiffuse.data(),
																 properties.mTextureDiffuse.size());
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to create nanogui::Texture: " << e.what() << "\n";
			return false;
		}
	} else {
		mTextureDiffuse.reset();
	}
	
	return true;
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
