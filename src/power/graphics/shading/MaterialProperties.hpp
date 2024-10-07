#pragma once

#include "filesystem/CompressedSerialization.hpp"
#include <nanogui/texture.h>
#include <glm/glm.hpp>
#include <functional> // For std::hash
#include <vector>
#include <memory>
#include <iostream>

struct MaterialProperties;

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
	
	SerializableMaterialProperties() = default;
	SerializableMaterialProperties(const SerializableMaterialProperties&) = delete;
	SerializableMaterialProperties& operator=(const SerializableMaterialProperties&) = delete;
	SerializableMaterialProperties(SerializableMaterialProperties&& other) noexcept;
	SerializableMaterialProperties& operator=(SerializableMaterialProperties&& other) noexcept;
	SerializableMaterialProperties(MaterialProperties& material);
	
	void serialize(CompressedSerialization::Serializer& serializer) const;
	bool deserialize(CompressedSerialization::Deserializer& deserializer);
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
	std::shared_ptr<nanogui::Texture> mTextureDiffuse;
	
	MaterialProperties() = default;
	MaterialProperties(MaterialProperties&& other) noexcept;
	MaterialProperties& operator=(MaterialProperties&& other) noexcept;
	
	MaterialProperties(const MaterialProperties&) = delete;
	MaterialProperties& operator=(const MaterialProperties&) = delete;
	
	bool deserialize(const SerializableMaterialProperties& properties);
	std::vector<uint8_t> downloadTexture();
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
