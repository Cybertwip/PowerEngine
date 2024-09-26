#pragma once

#include <vector>
#include <cstring> // For memcpy
#include <zlib.h>
#include <iostream>
#include <fstream> // For file I/O
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

// Utility class for serialization and deserialization with compression
class CompressedSerialization {
public:
	// Serializer component
	class Serializer {
	public:
		Serializer() = default;
		
		// Methods to write various data types
		void write_int32(int32_t value) {
			write_data(&value, sizeof(int32_t));
		}
		
		void write_uint32(uint32_t value) {
			write_data(&value, sizeof(uint32_t));
		}
		
		void write_uint64(uint64_t value) {
			write_data(&value, sizeof(uint64_t));
		}

		void write_float(float value) {
			write_data(&value, sizeof(float));
		}
		
		void write_double(double value) {
			write_data(&value, sizeof(double));
		}
		
		void write_string(const std::string& str) {
			int32_t length = static_cast<int32_t>(str.size());
			write_int32(length);
			write_data(str.c_str(), length);
		}
		
		void write_vec2(const glm::vec2& vec) {
			write_data(glm::value_ptr(vec), sizeof(float) * 2);
		}
		
		void write_vec3(const glm::vec3& vec) {
			write_data(glm::value_ptr(vec), sizeof(float) * 3);
		}
		
		void write_vec4(const glm::vec4& vec) {
			write_data(glm::value_ptr(vec), sizeof(float) * 4);
		}
		
		void write_quat(const glm::quat& quat) {
			write_data(glm::value_ptr(quat), sizeof(float) * 4);
		}
		
		void write_mat4(const glm::mat4& mat) {
			write_data(glm::value_ptr(mat), sizeof(float) * 16);
		}
		
		void write_bool(bool value) {
			char byte = value ? 1 : 0;
			write_data(&byte, sizeof(char));
		}
		
		// Method to add raw data
		void write_raw(const void* data, size_t size) {
			write_data(data, size);
		}
		
		// Finalize and get the compressed data
		bool get_compressed_data(std::vector<char>& outCompressedData) const {
			// Compress the buffer using zlib
			uLong sourceLen = static_cast<uLong>(buffer.size());
			uLong bound = compressBound(sourceLen);
			std::vector<Bytef> compressedData(bound);
			
			int res = compress(compressedData.data(), &bound, reinterpret_cast<const Bytef*>(buffer.data()), sourceLen);
			if (res != Z_OK) {
				std::cerr << "Compression failed with error code: " << res << "\n";
				return false;
			}
			
			// Resize to actual compressed size
			compressedData.resize(bound);
			
			// Prepare the output buffer: [sourceLen][compressedData]
			outCompressedData.resize(sizeof(uLong) + compressedData.size());
			std::memcpy(outCompressedData.data(), &sourceLen, sizeof(uLong));
			std::memcpy(outCompressedData.data() + sizeof(uLong), compressedData.data(), compressedData.size());
			
			return true;
		}
		
		// Save the serialized and compressed data to a file
		bool save_to_file(const std::string& filename) const {
			std::vector<char> compressedData;
			if (!get_compressed_data(compressedData)) {
				std::cerr << "Failed to get compressed data.\n";
				return false;
			}
			
			std::ofstream outFile(filename, std::ios::binary);
			if (!outFile) {
				std::cerr << "Failed to open file for writing: " << filename << "\n";
				return false;
			}
			
			outFile.write(compressedData.data(), compressedData.size());
			if (!outFile) {
				std::cerr << "Failed to write data to file: " << filename << "\n";
				return false;
			}
			
			outFile.close();
			if (!outFile) {
				std::cerr << "Error occurred while closing the file: " << filename << "\n";
				return false;
			}
			
			return true;
		}
		
	private:
		std::vector<char> buffer;
		
		// Helper method to write raw data to the buffer
		void write_data(const void* data, size_t size) {
			const char* bytes = static_cast<const char*>(data);
			buffer.insert(buffer.end(), bytes, bytes + size);
		}
	};
	
	// Deserializer component
	class Deserializer {
	public:
		// Initialize with compressed data
		bool initialize(const std::vector<char>& compressedData) {
			if (compressedData.size() < sizeof(uLong)) {
				std::cerr << "Compressed data is too small to contain uncompressed size.\n";
				return false;
			}
			
			// Read uncompressed size
			std::memcpy(&sourceLen, compressedData.data(), sizeof(uLong));
			
			// Read compressed data
			compressedDataPtr = reinterpret_cast<const Bytef*>(compressedData.data() + sizeof(uLong));
			compressedLen = compressedData.size() - sizeof(uLong);
			
			// Prepare buffer for decompressed data
			decompressedData.resize(sourceLen);
			uLong destLen = sourceLen;
			
			int res = uncompress(reinterpret_cast<Bytef*>(decompressedData.data()), &destLen,
								 compressedDataPtr, compressedLen);
			if (res != Z_OK) {
				std::cerr << "Decompression failed with error code: " << res << "\n";
				return false;
			}
			
			if (destLen != sourceLen) {
				std::cerr << "Decompressed data size mismatch. Expected: " << sourceLen
				<< ", Got: " << destLen << "\n";
				return false;
			}
			
			// Initialize read offset
			readOffset = 0;
			return true;
		}
		
		// Load compressed data from a file and initialize the deserializer
		bool load_from_file(const std::string& filename) {
			std::ifstream inFile(filename, std::ios::binary | std::ios::ate);
			if (!inFile) {
				std::cerr << "Failed to open file for reading: " << filename << "\n";
				return false;
			}
			
			std::streamsize size = inFile.tellg();
			if (size < static_cast<std::streamsize>(sizeof(uLong))) {
				std::cerr << "File too small to contain compressed data: " << filename << "\n";
				return false;
			}
			
			inFile.seekg(0, std::ios::beg);
			
			std::vector<char> compressedData(static_cast<size_t>(size));
			if (!inFile.read(compressedData.data(), size)) {
				std::cerr << "Failed to read data from file: " << filename << "\n";
				return false;
			}
			
			inFile.close();
			if (!inFile) {
				std::cerr << "Error occurred while closing the file: " << filename << "\n";
				return false;
			}
			
			return initialize(compressedData);
		}
		
		// Methods to read various data types
		bool read_int32(int32_t& value) {
			return read_data(&value, sizeof(int32_t));
		}
		
		bool read_uint32(uint32_t& value) {
			return read_data(&value, sizeof(uint32_t));
		}

		bool read_uint64(uint64_t& value) {
			return read_data(&value, sizeof(uint64_t));
		}

		bool read_float(float& value) {
			return read_data(&value, sizeof(float));
		}
		
		bool read_double(double& value) {
			return read_data(&value, sizeof(double));
		}
		
		bool read_string(std::string& str) {
			int32_t length;
			if (!read_int32(length)) return false;
			if (length < 0 || static_cast<size_t>(length) > (decompressedData.size() - readOffset)) {
				std::cerr << "Invalid string length: " << length << "\n";
				return false;
			}
			str.assign(decompressedData.data() + readOffset, length);
			readOffset += length;
			return true;
		}
		
		bool read_vec2(glm::vec2& vec) {
			return read_data(glm::value_ptr(vec), sizeof(float) * 2);
		}
		
		bool read_vec3(glm::vec3& vec) {
			return read_data(glm::value_ptr(vec), sizeof(float) * 3);
		}
		
		bool read_vec4(glm::vec4& vec) {
			return read_data(glm::value_ptr(vec), sizeof(float) * 4);
		}
		
		bool read_quat(glm::quat& quat) {
			return read_data(glm::value_ptr(quat), sizeof(float) * 4);
		}
		
		bool read_mat4(glm::mat4& mat) {
			return read_data(glm::value_ptr(mat), sizeof(float) * 16);
		}
		
		bool read_bool(bool& value) {
			char byte;
			if (!read_data(&byte, sizeof(char))) return false;
			value = (byte != 0);
			return true;
		}
		
		// Method to read raw data
		bool read_raw(void* data, size_t size) {
			return read_data(data, size);
		}
		
	private:
		uLong sourceLen = 0;
		const Bytef* compressedDataPtr = nullptr;
		uLong compressedLen = 0;
		std::vector<char> decompressedData;
		size_t readOffset = 0;
		
		// Helper method to read raw data from the decompressed buffer
		bool read_data(void* data, size_t size) {
			if (readOffset + size > decompressedData.size()) {
				std::cerr << "Attempt to read beyond buffer size.\n";
				return false;
			}
			std::memcpy(data, decompressedData.data() + readOffset, size);
			readOffset += size;
			return true;
		}
	};
};
