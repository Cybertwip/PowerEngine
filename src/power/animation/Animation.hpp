#pragma once

#include <vector>
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <algorithm>
#include <string>
#include <fstream>
#include <cstring> // For memcpy
#include <zlib.h>
#include <iostream> // For error messages

class Animation {
public:
	struct KeyFrame {
		float time;
		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;
	};
	
	struct BoneAnimation {
		int bone_index;
		std::vector<KeyFrame> keyframes;
	};
	
	// Add keyframes for a specific bone
	void add_bone_keyframes(int boneIndex, const std::vector<KeyFrame>& keyframes) {
		m_bone_animations.push_back({boneIndex, keyframes});
	}
	
	void sort() {
		std::sort(m_bone_animations.begin(), m_bone_animations.end(), [](const BoneAnimation& a, const BoneAnimation& b) {
			return a.bone_index < b.bone_index;
		});
	}
	
	// Get keyframes for a bone by name
	const std::vector<KeyFrame>* get_bone_keyframes(int boneIndex) const {
		for (const auto& bone_anim : m_bone_animations) {
			if (bone_anim.bone_index == boneIndex) {
				return &bone_anim.keyframes;
			}
		}
		return nullptr;  // Bone not found
	}
	
	// Set the total duration of the animation
	void set_duration(int duration) {
		m_duration = duration;
	}
	
	// Get the total duration of the animation
	int get_duration() const {
		return m_duration;
	}
	
	// Evaluate the animation for a specific time, returning the transform for each bone
	std::vector<glm::mat4> evaluate(float time) const {
		std::vector<glm::mat4> bone_transforms;
		
		// For each bone animation
		for (const auto& bone_anim : m_bone_animations) {
			const auto& keyframes = bone_anim.keyframes;
			
			// If there are no keyframes, continue to the next bone
			if (keyframes.empty()) {
				continue;
			}
			
			// If the time is before the first keyframe, return the first keyframe
			if (time <= keyframes.front().time) {
				const KeyFrame& first_keyframe = keyframes.front();
				
				glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), first_keyframe.translation);
				glm::mat4 rotation_matrix = glm::mat4_cast(first_keyframe.rotation);
				glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), first_keyframe.scale);
				
				glm::mat4 bone_transform = translation_matrix * rotation_matrix * scale_matrix;
				bone_transforms.push_back(bone_transform);
				continue;
			}
			
			// If the time is after the last keyframe, return the last keyframe
			if (time >= keyframes.back().time) {
				const KeyFrame& last_keyframe = keyframes.back();
				
				glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), last_keyframe.translation);
				glm::mat4 rotation_matrix = glm::mat4_cast(last_keyframe.rotation);
				glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), last_keyframe.scale);
				
				glm::mat4 bone_transform = translation_matrix * rotation_matrix * scale_matrix;
				bone_transforms.push_back(bone_transform);
				continue;
			}
			
			// Find the two keyframes surrounding the given time
			auto it1 = std::find_if(keyframes.begin(), keyframes.end(), [&](const KeyFrame& kf) {
				return kf.time >= time;
			});
			
			auto it0 = (it1 != keyframes.begin()) ? std::prev(it1) : it1;
			
			// Interpolate between keyframes
			float t = (time - it0->time) / (it1->time - it0->time);
			
			glm::vec3 interpolated_translation = glm::mix(it0->translation, it1->translation, t);
			glm::quat interpolated_rotation = glm::slerp(it0->rotation, it1->rotation, t);
			glm::vec3 interpolated_scale = glm::mix(it0->scale, it1->scale, t);
			
			// Construct the transformation matrix
			glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), interpolated_translation);
			glm::mat4 rotation_matrix = glm::mat4_cast(interpolated_rotation);
			glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), interpolated_scale);
			
			glm::mat4 bone_transform = translation_matrix * rotation_matrix * scale_matrix;
			bone_transforms.push_back(bone_transform);
		}
		
		return bone_transforms;
	}
	
	/**
	 * @brief Saves the animation to a .pan file with compression.
	 *
	 * @param filename The path to the file where the animation will be saved.
	 * @return true If the file was saved successfully.
	 * @return false If an error occurred during saving.
	 */
	bool save_to_file(const std::string& filename) const {
		// Serialize the animation data into a buffer
		std::vector<char> buffer;
		
		// Helper lambda to write primitive data types to the buffer
		auto write_data = [&](const void* data, size_t size) {
			const char* bytes = static_cast<const char*>(data);
			buffer.insert(buffer.end(), bytes, bytes + size);
		};
		
		// Write header
		const char header[4] = {'P', 'A', 'N', '1'};
		write_data(header, sizeof(header));
		
		// Write duration (int32)
		int32_t duration = static_cast<int32_t>(m_duration);
		write_data(&duration, sizeof(int32_t));
		
		// Write number of BoneAnimations (int32)
		int32_t numBoneAnims = static_cast<int32_t>(m_bone_animations.size());
		write_data(&numBoneAnims, sizeof(int32_t));
		
		// Serialize each BoneAnimation
		for (const BoneAnimation& bone_anim : m_bone_animations) {
			// Write bone_index (int32)
			write_data(&bone_anim.bone_index, sizeof(int32_t));
			
			// Write number of KeyFrames (int32)
			int32_t numKeyFrames = static_cast<int32_t>(bone_anim.keyframes.size());
			write_data(&numKeyFrames, sizeof(int32_t));
			
			// Serialize each KeyFrame
			for (const KeyFrame& keyframe : bone_anim.keyframes) {
				// Write time (float)
				write_data(&keyframe.time, sizeof(float));
				
				// Write translation (3 floats)
				write_data(glm::value_ptr(keyframe.translation), sizeof(float) * 3);
				
				// Write rotation (4 floats for quaternion)
				write_data(glm::value_ptr(keyframe.rotation), sizeof(float) * 4);
				
				// Write scale (3 floats)
				write_data(glm::value_ptr(keyframe.scale), sizeof(float) * 3);
			}
		}
		
		// Compress the serialized data using zlib
		uLong sourceLen = static_cast<uLong>(buffer.size());
		uLong bound = compressBound(sourceLen);
		std::vector<Bytef> compressedData(bound);
		
		int res = compress(compressedData.data(), &bound, reinterpret_cast<Bytef*>(buffer.data()), sourceLen);
		if (res != Z_OK) {
			// Compression failed
			std::cerr << "Compression failed with error code: " << res << "\n";
			return false;
		}
		
		// Open the file for binary writing
		std::ofstream outFile(filename, std::ios::binary);
		if (!outFile) {
			// File opening failed
			std::cerr << "Failed to open file for writing: " << filename << "\n";
			return false;
		}
		
		// Write the original size (uncompressed)
		outFile.write(reinterpret_cast<const char*>(&sourceLen), sizeof(uLong));
		
		// Write the compressed size
		outFile.write(reinterpret_cast<const char*>(&bound), sizeof(uLong));
		
		// Write the compressed data
		outFile.write(reinterpret_cast<const char*>(compressedData.data()), bound);
		
		// Close the file
		outFile.close();
		
		return true;
	}
	
	/**
	 * @brief Loads the animation from a .pan file.
	 *
	 * @param filename The path to the .pan file to load.
	 * @return true If the file was loaded successfully.
	 * @return false If an error occurred during loading.
	 */
	bool load_from_file(const std::string& filename) {
		// Open the file for binary reading
		std::ifstream inFile(filename, std::ios::binary | std::ios::ate);
		if (!inFile) {
			std::cerr << "Failed to open file for reading: " << filename << "\n";
			return false;
		}
		
		// Get the file size
		std::streamsize fileSize = inFile.tellg();
		inFile.seekg(0, std::ios::beg);
		
		// Read the entire file into a buffer
		std::vector<char> fileBuffer(fileSize);
		if (!inFile.read(fileBuffer.data(), fileSize)) {
			std::cerr << "Failed to read file: " << filename << "\n";
			return false;
		}
		inFile.close();
		
		size_t offset = 0;
		
		// Helper lambda to read primitive data types from the buffer
		auto read_data = [&](void* data, size_t size) -> bool {
			if (offset + size > fileBuffer.size()) {
				return false; // Not enough data
			}
			std::memcpy(data, fileBuffer.data() + offset, size);
			offset += size;
			return true;
		};
		
		// Read and verify header
		char header[4];
		if (!read_data(header, sizeof(header))) {
			std::cerr << "Failed to read header.\n";
			return false;
		}
		if (std::strncmp(header, "PAN1", 4) != 0) {
			std::cerr << "Invalid file header. Expected 'PAN1'.\n";
			return false;
		}
		
		// Read original data size
		uLong sourceLen;
		if (!read_data(&sourceLen, sizeof(uLong))) {
			std::cerr << "Failed to read original data size.\n";
			return false;
		}
		
		// Read compressed data size
		uLong compressedLen;
		if (!read_data(&compressedLen, sizeof(uLong))) {
			std::cerr << "Failed to read compressed data size.\n";
			return false;
		}
		
		// Check if the remaining data is at least compressedLen bytes
		if (offset + compressedLen > fileBuffer.size()) {
			std::cerr << "Compressed data size exceeds file size.\n";
			return false;
		}
		
		// Read compressed data
		std::vector<Bytef> compressedData(compressedLen);
		if (!read_data(compressedData.data(), compressedLen)) {
			std::cerr << "Failed to read compressed data.\n";
			return false;
		}
		
		// Decompress the data
		std::vector<char> decompressedData(sourceLen);
		uLong destLen = sourceLen;
		int res = uncompress(reinterpret_cast<Bytef*>(decompressedData.data()), &destLen,
							 compressedData.data(), compressedLen);
		if (res != Z_OK) {
			std::cerr << "Decompression failed with error code: " << res << "\n";
			return false;
		}
		
		if (destLen != sourceLen) {
			std::cerr << "Decompressed data size mismatch.\n";
			return false;
		}
		
		// Now deserialize the decompressedData buffer
		size_t dataOffset = 0;
		
		// Helper lambda to read primitive data types from the decompressed buffer
		auto read_decompressed = [&](void* data, size_t size) -> bool {
			if (dataOffset + size > decompressedData.size()) {
				return false; // Not enough data
			}
			std::memcpy(data, decompressedData.data() + dataOffset, size);
			dataOffset += size;
			return true;
		};
		
		// Read duration
		int32_t duration;
		if (!read_decompressed(&duration, sizeof(int32_t))) {
			std::cerr << "Failed to read duration.\n";
			return false;
		}
		m_duration = duration;
		
		// Read number of BoneAnimations
		int32_t numBoneAnims;
		if (!read_decompressed(&numBoneAnims, sizeof(int32_t))) {
			std::cerr << "Failed to read number of bone animations.\n";
			return false;
		}
		
		// Clear existing BoneAnimations
		m_bone_animations.clear();
		m_bone_animations.reserve(numBoneAnims);
		
		// Read each BoneAnimation
		for (int i = 0; i < numBoneAnims; ++i) {
			BoneAnimation bone_anim;
			
			// Read bone_index
			if (!read_decompressed(&bone_anim.bone_index, sizeof(int32_t))) {
				std::cerr << "Failed to read bone_index for BoneAnimation " << i << ".\n";
				return false;
			}
			
			// Read number of KeyFrames
			int32_t numKeyFrames;
			if (!read_decompressed(&numKeyFrames, sizeof(int32_t))) {
				std::cerr << "Failed to read number of keyframes for BoneAnimation " << i << ".\n";
				return false;
			}
			
			if (numKeyFrames < 0) {
				std::cerr << "Invalid number of keyframes for BoneAnimation " << i << ".\n";
				return false;
			}
			
			bone_anim.keyframes.reserve(numKeyFrames);
			
			// Read each KeyFrame
			for (int j = 0; j < numKeyFrames; ++j) {
				KeyFrame keyframe;
				
				// Read time
				if (!read_decompressed(&keyframe.time, sizeof(float))) {
					std::cerr << "Failed to read time for KeyFrame " << j << " of BoneAnimation " << i << ".\n";
					return false;
				}
				
				// Read translation (3 floats)
				if (!read_decompressed(glm::value_ptr(keyframe.translation), sizeof(float) * 3)) {
					std::cerr << "Failed to read translation for KeyFrame " << j << " of BoneAnimation " << i << ".\n";
					return false;
				}
				
				// Read rotation (4 floats)
				if (!read_decompressed(glm::value_ptr(keyframe.rotation), sizeof(float) * 4)) {
					std::cerr << "Failed to read rotation for KeyFrame " << j << " of BoneAnimation " << i << ".\n";
					return false;
				}
				
				// Read scale (3 floats)
				if (!read_decompressed(glm::value_ptr(keyframe.scale), sizeof(float) * 3)) {
					std::cerr << "Failed to read scale for KeyFrame " << j << " of BoneAnimation " << i << ".\n";
					return false;
				}
				
				bone_anim.keyframes.push_back(keyframe);
			}
			
			m_bone_animations.push_back(bone_anim);
		}
		
		// Optionally, sort the bone animations by bone_index if not already sorted
		// This step is optional and depends on how you intend to use the data
		// sort();
		
		return true;
	}


private:
	std::vector<BoneAnimation> m_bone_animations;
	int m_duration = 0;  // Duration of the animation
};
