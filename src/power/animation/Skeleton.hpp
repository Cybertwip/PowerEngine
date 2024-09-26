#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>

#include <fstream>
#include <cstring> // For memcpy
#include <zlib.h>
#include <iostream> // For error messages

class Skeleton {
public:
	struct Bone {
		std::string name;
		int index;  // -1 if root
		int parent_index;  // -1 if root
		glm::mat4 offset;
		glm::mat4 bindpose;
		glm::mat4 local;
		glm::mat4 global;
		glm::mat4 transform;
		std::vector<int> children;
		
		Bone(){
			
		}

		Bone(const std::string& name, int index, int parent_index, const glm::mat4& offset, const glm::mat4& bindpose, const glm::mat4& local)
		: name(name), index(index), parent_index(parent_index), offset(offset), bindpose(bindpose), local(local), global(1.0f), transform(1.0f) {}
	};
	
	
	Skeleton() = default;
	
	void add_bone(const std::string& name, const glm::mat4& offset, const glm::mat4& bindpose, int parent_index) {
		int new_bone_index = static_cast<int>(m_bones.size());
		m_bones.emplace_back(name, new_bone_index, parent_index, offset, bindpose, glm::identity<glm::mat4>());
		
		if (parent_index != -1) {
			assert(parent_index >= 0 && parent_index < new_bone_index);
			m_bones[parent_index].children.push_back(new_bone_index);
		}
	}
	
	// Get the number of bones
	int num_bones() const {
		return static_cast<int>(m_bones.size());
	}
	
	// Get bone by index
	const Bone& get_bone(int index) const {
		assert(index >= 0 && index < num_bones());
		return m_bones[index];
	}
	
	// Get mutable bone by index
	Bone& get_bone(int index) {
		assert(index >= 0 && index < num_bones());
		return m_bones[index];
	}
	
	// Find bone by name
	Bone* find_bone(const std::string& name) {
		for (auto& bone : m_bones) {
			if (bone.name == name) {
				return &bone;
			}
		}
		return nullptr;
	}
	
	void compute_offsets(const std::vector<glm::mat4>& withAnimation) {
		if (m_bones.empty()) return;
		
		if (!withAnimation.empty()) {
			assert(withAnimation.size() == m_bones.size() && "Unmatched animations and bones size");
		}
		
		for (Bone& bone : m_bones) {
			if (bone.parent_index == -1) {
				glm::mat4 identity = glm::mat4(1.0f);
				compute_global_and_transform(bone, identity, withAnimation);
			}
		}
	}
	
	/**
	 * @brief Saves the skeleton to a .psk file with compression.
	 *
	 * @param filename The path to the file where the skeleton will be saved.
	 * @return true If the file was saved successfully.
	 * @return false If an error occurred during saving.
	 */
	bool save_to_file(const std::string& filename) const {
		// Serialize the skeleton data into a buffer
		std::vector<char> buffer;
		
		// Helper lambda to write primitive data types to the buffer
		auto write_data = [&](const void* data, size_t size) {
			const char* bytes = static_cast<const char*>(data);
			buffer.insert(buffer.end(), bytes, bytes + size);
		};
		
		// Write header
		const char header[4] = {'P', 'S', 'K', '1'};
		write_data(header, sizeof(header));
		
		// Write number of bones (int32)
		int32_t numBones = static_cast<int32_t>(m_bones.size());
		write_data(&numBones, sizeof(int32_t));
		
		// Serialize each bone
		for (const Bone& bone : m_bones) {
			// Write name length and name characters
			int32_t nameLength = static_cast<int32_t>(bone.name.size());
			write_data(&nameLength, sizeof(int32_t));
			write_data(bone.name.c_str(), nameLength);
			
			// Write index and parent_index
			write_data(&bone.index, sizeof(int32_t));
			write_data(&bone.parent_index, sizeof(int32_t));
			
			// Write offset matrix
			write_data(glm::value_ptr(bone.offset), sizeof(float) * 16);
			
			// Write bindpose matrix
			write_data(glm::value_ptr(bone.bindpose), sizeof(float) * 16);
			
			// Write local matrix
			write_data(glm::value_ptr(bone.local), sizeof(float) * 16);
			
			// Write global matrix
			write_data(glm::value_ptr(bone.global), sizeof(float) * 16);
			
			// Write transform matrix
			write_data(glm::value_ptr(bone.transform), sizeof(float) * 16);
			
			// Write number of children
			int32_t numChildren = static_cast<int32_t>(bone.children.size());
			write_data(&numChildren, sizeof(int32_t));
			
			// Write children indices
			for (int childIndex : bone.children) {
				write_data(&childIndex, sizeof(int32_t));
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
	 * @brief Loads the skeleton from a .psk file.
	 *
	 * @param filename The path to the .psk file to load.
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
		if (std::strncmp(header, "PSK1", 4) != 0) {
			std::cerr << "Invalid file header. Expected 'PSK1'.\n";
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
		
		// Read number of bones
		int32_t numBones;
		if (!read_decompressed(&numBones, sizeof(int32_t))) {
			std::cerr << "Failed to read number of bones.\n";
			return false;
		}
		
		// Clear existing bones
		m_bones.clear();
		m_bones.reserve(numBones);
		
		// Temporary storage for bones to allow setting up children after all bones are read
		std::vector<Bone> tempBones(numBones);
		
		// Read each bone
		for (int i = 0; i < numBones; ++i) {
			Bone bone;
			
			// Read name length
			int32_t nameLength;
			if (!read_decompressed(&nameLength, sizeof(int32_t))) {
				std::cerr << "Failed to read bone name length for bone " << i << ".\n";
				return false;
			}
			
			// Read name characters
			if (nameLength < 0 || static_cast<size_t>(nameLength) > (decompressedData.size() - dataOffset)) {
				std::cerr << "Invalid bone name length for bone " << i << ".\n";
				return false;
			}
			bone.name.assign(decompressedData.data() + dataOffset, nameLength);
			dataOffset += nameLength;
			
			// Read index and parent_index
			if (!read_decompressed(&bone.index, sizeof(int32_t)) ||
				!read_decompressed(&bone.parent_index, sizeof(int32_t))) {
				std::cerr << "Failed to read bone indices for bone " << i << ".\n";
				return false;
			}
			
			// Read offset matrix
			if (!read_decompressed(glm::value_ptr(bone.offset), sizeof(float) * 16)) {
				std::cerr << "Failed to read offset matrix for bone " << i << ".\n";
				return false;
			}
			
			// Read bindpose matrix
			if (!read_decompressed(glm::value_ptr(bone.bindpose), sizeof(float) * 16)) {
				std::cerr << "Failed to read bindpose matrix for bone " << i << ".\n";
				return false;
			}
			
			// Read local matrix
			if (!read_decompressed(glm::value_ptr(bone.local), sizeof(float) * 16)) {
				std::cerr << "Failed to read local matrix for bone " << i << ".\n";
				return false;
			}
			
			// Read global matrix
			if (!read_decompressed(glm::value_ptr(bone.global), sizeof(float) * 16)) {
				std::cerr << "Failed to read global matrix for bone " << i << ".\n";
				return false;
			}
			
			// Read transform matrix
			if (!read_decompressed(glm::value_ptr(bone.transform), sizeof(float) * 16)) {
				std::cerr << "Failed to read transform matrix for bone " << i << ".\n";
				return false;
			}
			
			// Read number of children
			int32_t numChildren;
			if (!read_decompressed(&numChildren, sizeof(int32_t))) {
				std::cerr << "Failed to read number of children for bone " << i << ".\n";
				return false;
			}
			
			// Read children indices
			if (numChildren < 0) {
				std::cerr << "Invalid number of children for bone " << i << ".\n";
				return false;
			}
			bone.children.reserve(numChildren);
			for (int j = 0; j < numChildren; ++j) {
				int32_t childIndex;
				if (!read_decompressed(&childIndex, sizeof(int32_t))) {
					std::cerr << "Failed to read child index " << j << " for bone " << i << ".\n";
					return false;
				}
				bone.children.push_back(childIndex);
			}
			
			tempBones[i] = bone;
		}
		
		// Assign the deserialized bones to m_bones
		m_bones = std::move(tempBones);
		
		// Optionally, verify the integrity of the skeleton (e.g., no cycles, valid parent indices)
		// This step is skipped for brevity but recommended in a production environment.
		
		return true;
	}

	
private:
	std::vector<Bone> m_bones;
	
	void compute_global_and_transform(Bone& bone, const glm::mat4& parentGlobal, const std::vector<glm::mat4>& withAnimation) {
		
		// Start with the parent's global transformation
		glm::mat4 global = parentGlobal;
		
		// Apply the bone's bind pose first
		global *= bone.bindpose;
		
		// Only apply the special rotation to bone index 3
		glm::mat4 transformation = glm::mat4(1.0f);

		if (!withAnimation.empty()) {
			transformation = withAnimation[bone.index];
		}
				
		global *= bone.local * transformation;
		
		bone.transform = global * bone.offset;
		
		for (int childIndex : bone.children) {
			compute_global_and_transform(m_bones[childIndex], global, withAnimation);
		}
	}
	
};
