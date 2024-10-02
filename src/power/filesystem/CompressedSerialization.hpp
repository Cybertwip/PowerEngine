#pragma once

#include <numeric>
#include <vector>
#include <cstring> // For memcpy
#include <zlib.h>
#include <iostream>
#include <fstream> // For file I/O
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <thread>
#include <future>
#include <mutex>

// Utility class for serialization and deserialization with compression
class CompressedSerialization {
public:
	// Define the magic number as a static constant (4 bytes)
	static constexpr const char MAGIC_NUMBER[4] = { 'C', 'S', 'P', 'R' };
	
	// Serializer component
	class Serializer {
	public:
		Serializer() = default;
		
		// Methods to write various data types to the main buffer
		void write_int32(int32_t value) {
			write_data(&value, sizeof(int32_t), buffer);
		}
		
		void write_uint32(uint32_t value) {
			write_data(&value, sizeof(uint32_t), buffer);
		}
		
		void write_uint64(uint64_t value) {
			write_data(&value, sizeof(uint64_t), buffer);
		}
		
		void write_float(float value) {
			write_data(&value, sizeof(float), buffer);
		}
		
		void write_double(double value) {
			write_data(&value, sizeof(double), buffer);
		}
		
		void write_string(const std::string& str) {
			int32_t length = static_cast<int32_t>(str.size());
			write_int32(length);
			write_data(str.c_str(), length, buffer);
		}
		
		void write_vec2(const glm::vec2& vec) {
			write_data(glm::value_ptr(vec), sizeof(float) * 2, buffer);
		}
		
		void write_vec3(const glm::vec3& vec) {
			write_data(glm::value_ptr(vec), sizeof(float) * 3, buffer);
		}
		
		void write_vec4(const glm::vec4& vec) {
			write_data(glm::value_ptr(vec), sizeof(float) * 4, buffer);
		}
		
		void write_quat(const glm::quat& quat) {
			write_data(glm::value_ptr(quat), sizeof(float) * 4, buffer);
		}
		
		void write_mat4(const glm::mat4& mat) {
			write_data(glm::value_ptr(mat), sizeof(float) * 16, buffer);
		}
		
		void write_bool(bool value) {
			char byte = value ? 1 : 0;
			write_data(&byte, sizeof(char), buffer);
		}
		
		// Methods to write various data types to the header
		void write_header_int32(int32_t value) {
			write_data(&value, sizeof(int32_t), header);
		}
		
		void write_header_uint32(uint32_t value) {
			write_data(&value, sizeof(uint32_t), header);
		}
		
		void write_header_uint64(uint64_t value) {
			write_data(&value, sizeof(uint64_t), header);
		}
		
		void write_header_float(float value) {
			write_data(&value, sizeof(float), header);
		}
		
		void write_header_double(double value) {
			write_data(&value, sizeof(double), header);
		}
		
		void write_header_string(const std::string& str) {
			int32_t length = static_cast<int32_t>(str.size());
			write_header_int32(length);
			write_data(str.c_str(), length, header);
		}
		
		void write_header_vec2(const glm::vec2& vec) {
			write_data(glm::value_ptr(vec), sizeof(float) * 2, header);
		}
		
		void write_header_vec3(const glm::vec3& vec) {
			write_data(glm::value_ptr(vec), sizeof(float) * 3, header);
		}
		
		void write_header_vec4(const glm::vec4& vec) {
			write_data(glm::value_ptr(vec), sizeof(float) * 4, header);
		}
		
		void write_header_quat(const glm::quat& quat) {
			write_data(glm::value_ptr(quat), sizeof(float) * 4, header);
		}
		
		void write_header_mat4(const glm::mat4& mat) {
			write_data(glm::value_ptr(mat), sizeof(float) * 16, header);
		}
		
		void write_header_bool(bool value) {
			char byte = value ? 1 : 0;
			write_data(&byte, sizeof(char), header);
		}
		
		// Method to add raw data to the main buffer
		void write_raw(const void* data, size_t size) {
			write_data(data, size, buffer);
		}
		
		// Method to add raw data to the header
		void write_header_raw(const void* data, size_t size) {
			write_data(data, size, header);
		}
		
		// Finalize and write the compressed data directly to a stream using multithreading
		bool get_compressed_data(std::ostream& outStream) const {
			// Write Magic Number
			outStream.write(MAGIC_NUMBER, sizeof(MAGIC_NUMBER));
			if (!outStream) {
				std::cerr << "Failed to write magic number to stream.\n";
				return false;
			}
			
			// Write Header Size
			uint32_t headerSize = static_cast<uint32_t>(header.size());
			outStream.write(reinterpret_cast<const char*>(&headerSize), sizeof(uint32_t));
			if (!outStream) {
				std::cerr << "Failed to write header size to stream.\n";
				return false;
			}
			
			// Write Header Data
			if (headerSize > 0) {
				outStream.write(header.data(), header.size());
				if (!outStream) {
					std::cerr << "Failed to write header data to stream.\n";
					return false;
				}
			}
			
			// Compress the main buffer
			// Determine the number of available hardware threads
			unsigned int numThreads = std::thread::hardware_concurrency();
			if (numThreads == 0) numThreads = 4; // Fallback to 4 threads if unable to detect
			numThreads = 1;
			// Calculate chunk sizes
			size_t totalSize = buffer.size();
			size_t chunkSize = totalSize / numThreads;
			if (chunkSize == 0) {
				numThreads = 1;
				chunkSize = totalSize;
			}
			
			// Containers for compressed chunks and their sizes
			std::vector<std::vector<Bytef>> compressedChunks(numThreads);
			std::vector<uLong> compressedSizes(numThreads, 0);
			std::vector<uLong> uncompressedSizes(numThreads, 0); // Store uncompressed sizes
			std::vector<std::future<bool>> futures;
			
			// Lambda function to compress a chunk
			auto compressChunk = [&](unsigned int threadIndex, const char* data, size_t size) -> bool {
				if (size == 0) {
					compressedSizes[threadIndex] = 0;
					return true;
				}
				uLongf bound = compressBound(size);
				compressedChunks[threadIndex].resize(bound);
				int res = compress(compressedChunks[threadIndex].data(), &bound, reinterpret_cast<const Bytef*>(data), size);
				if (res != Z_OK) {
					std::cerr << "Compression failed in thread " << threadIndex << " with error code: " << res << "\n";
					return false;
				}
				compressedChunks[threadIndex].resize(bound);
				compressedSizes[threadIndex] = bound;
				uncompressedSizes[threadIndex] = static_cast<uLong>(size); // Store uncompressed size
				return true;
			};
			
			// Launch compression tasks
			for (unsigned int i = 0; i < numThreads; ++i) {
				size_t offset = i * chunkSize;
				size_t currentChunkSize = (i == numThreads - 1) ? (totalSize - offset) : chunkSize;
				const char* dataPtr = buffer.data() + offset;
				
				futures.emplace_back(std::async(std::launch::async, compressChunk, i, dataPtr, currentChunkSize));
			}
			
			// Wait for all compression tasks to complete
			for (auto& fut : futures) {
				if (!fut.get()) {
					std::cerr << "One of the compression threads failed.\n";
					return false;
				}
			}
			
			// Write compression metadata and compressed data to the stream
			// Structure:
			// [numThreads][compressedSizes][uncompressedSizes][compressedData]
			
			// Write number of threads
			unsigned int numThreadsStored = static_cast<unsigned int>(numThreads);
			outStream.write(reinterpret_cast<const char*>(&numThreadsStored), sizeof(unsigned int));
			if (!outStream) {
				std::cerr << "Failed to write number of threads to stream.\n";
				return false;
			}
			
			// Write each compressed chunk size
			for (unsigned int i = 0; i < numThreads; ++i) {
				outStream.write(reinterpret_cast<const char*>(&compressedSizes[i]), sizeof(uLong));
				if (!outStream) {
					std::cerr << "Failed to write compressed chunk size to stream.\n";
					return false;
				}
			}
			
			// Write each uncompressed chunk size
			for (unsigned int i = 0; i < numThreads; ++i) {
				outStream.write(reinterpret_cast<const char*>(&uncompressedSizes[i]), sizeof(uLong));
				if (!outStream) {
					std::cerr << "Failed to write uncompressed chunk size to stream.\n";
					return false;
				}
			}
			
			// Write all compressed chunks
			for (unsigned int i = 0; i < numThreads; ++i) {
				if (compressedSizes[i] > 0) {
					outStream.write(reinterpret_cast<const char*>(compressedChunks[i].data()), compressedSizes[i]);
					if (!outStream) {
						std::cerr << "Failed to write compressed chunk data to stream.\n";
						return false;
					}
				}
			}
			
			return true;
		}
		
		// Save the serialized and compressed data to a file with magic number and header
		bool save_to_file(const std::string& filename) const {
			std::ofstream outFile(filename, std::ios::out | std::ios::binary);
			if (!outFile) {
				std::cerr << "Failed to open file for writing: " << filename << "\n";
				return false;
			}
			
			// Get and write all data, including magic number and header
			if (!get_compressed_data(outFile)) {
				std::cerr << "Failed to get compressed data.\n";
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
		std::vector<char> buffer; // Main buffer
		std::vector<char> header; // Header buffer
		
		// Helper method to write raw data to a specified buffer
		void write_data(const void* data, size_t size, std::vector<char>& targetBuffer) const {
			const char* bytes = static_cast<const char*>(data);
			targetBuffer.insert(targetBuffer.end(), bytes, bytes + size);
		}
	};
	
	// Deserializer component
	class Deserializer {
	public:
		Deserializer() = default;
		
		// Initialize by reading from a stream
		bool initialize(std::istream& inStream) {
			// Read and verify Magic Number
			char fileMagicNumber[4];
			inStream.read(fileMagicNumber, sizeof(fileMagicNumber));
			if (inStream.gcount() != sizeof(fileMagicNumber)) {
				std::cerr << "Failed to read magic number.\n";
				return false;
			}
			if (std::memcmp(fileMagicNumber, MAGIC_NUMBER, sizeof(MAGIC_NUMBER)) != 0) {
				std::cerr << "Magic number mismatch. This file is not from this program.\n";
				return false;
			}
			
			// Read Header Size
			uint32_t headerSize = 0;
			inStream.read(reinterpret_cast<char*>(&headerSize), sizeof(uint32_t));
			if (inStream.gcount() != sizeof(uint32_t)) {
				std::cerr << "Failed to read header size.\n";
				return false;
			}
			
			// Read Header Data
			header.clear();
			if (headerSize > 0) {
				header.resize(headerSize);
				inStream.read(header.data(), headerSize);
				if (static_cast<uint32_t>(inStream.gcount()) != headerSize) {
					std::cerr << "Failed to read header data.\n";
					return false;
				}
			}
			
			// Read the rest of the stream into a vector
			std::vector<char> compressedData((std::istreambuf_iterator<char>(inStream)),
											 std::istreambuf_iterator<char>());
			
			// Ensure there's compressed data to process
			if (compressedData.empty()) {
				std::cerr << "No compressed data found.\n";
				return false;
			}
			
			// Decompress the data
			return decompress_data(compressedData);
		}
		
		// Load compressed data from a file and initialize the deserializer
		bool load_from_file(const std::string& filename) {
			std::ifstream inFile(filename, std::ios::binary);
			if (!inFile) {
				std::cerr << "Failed to open file for reading: " << filename << "\n";
				return false;
			}
			
			// Initialize the stream-based initialization
			bool result = initialize(inFile);
			inFile.close();
			if (!result) {
				std::cerr << "Failed to initialize deserializer from file: " << filename << "\n";
				return false;
			}
			
			return true;
		}
		
		// Methods to read various data types from the main buffer
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
			if (length < 0 || static_cast<size_t>(length) > (decompressedData.size() - readOffsetTotal)) {
				std::cerr << "Invalid string length: " << length << "\n";
				return false;
			}
			str.assign(decompressedData.data() + readOffsetTotal, length);
			readOffsetTotal += length;
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
		
		// Method to read raw data from the main buffer
		bool read_raw(void* data, size_t size) {
			return read_data(data, size);
		}
		
		// Methods to read various data types from the header
		bool read_header_int32(int32_t& value) {
			return read_header_data(&value, sizeof(int32_t));
		}
		
		bool read_header_uint32(uint32_t& value) {
			return read_header_data(&value, sizeof(uint32_t));
		}
		
		bool read_header_uint64(uint64_t& value) {
			return read_header_data(&value, sizeof(uint64_t));
		}
		
		bool read_header_float(float& value) {
			return read_header_data(&value, sizeof(float));
		}
		
		bool read_header_double(double& value) {
			return read_header_data(&value, sizeof(double));
		}
		
		bool read_header_string(std::string& str) {
			int32_t length;
			if (!read_header_int32(length)) return false;
			if (length < 0 || static_cast<size_t>(length) > (header.size() - readHeaderOffset)) {
				std::cerr << "Invalid header string length: " << length << "\n";
				return false;
			}
			str.assign(header.data() + readHeaderOffset, length);
			readHeaderOffset += length;
			return true;
		}
		
		bool read_header_vec2(glm::vec2& vec) {
			return read_header_data(glm::value_ptr(vec), sizeof(float) * 2);
		}
		
		bool read_header_vec3(glm::vec3& vec) {
			return read_header_data(glm::value_ptr(vec), sizeof(float) * 3);
		}
		
		bool read_header_vec4(glm::vec4& vec) {
			return read_header_data(glm::value_ptr(vec), sizeof(float) * 4);
		}
		
		bool read_header_quat(glm::quat& quat) {
			return read_header_data(glm::value_ptr(quat), sizeof(float) * 4);
		}
		
		bool read_header_mat4(glm::mat4& mat) {
			return read_header_data(glm::value_ptr(mat), sizeof(float) * 16);
		}
		
		bool read_header_bool(bool& value) {
			char byte;
			if (!read_header_data(&byte, sizeof(char))) return false;
			value = (byte != 0);
			return true;
		}
		
		// Method to read raw data from the header
		bool read_header_raw(void* data, size_t size) {
			return read_header_data(data, size);
		}
		
		// Retrieve the header as a string (for verification or other purposes)
		std::string get_header_string() const {
			return std::string(header.begin(), header.end());
		}
		
	private:
		unsigned int numThreads = 1;
		std::vector<uLong> compressedSizes;
		std::vector<uLong> uncompressedSizes; // Store uncompressed sizes per chunk
		std::vector<std::vector<Bytef>> compressedChunks;
		std::vector<std::vector<char>> decompressedChunks;
		std::vector<char> decompressedData;
		size_t readOffsetTotal = 0;
		
		std::vector<char> header; // Header buffer
		size_t readHeaderOffset = 0;
		
		// Helper method to read raw data from the main decompressed buffer
		bool read_data(void* data, size_t size) {
			if (readOffsetTotal + size > decompressedData.size()) {
				std::cerr << "Attempt to read beyond buffer size.\n";
				return false;
			}
			std::memcpy(data, decompressedData.data() + readOffsetTotal, size);
			readOffsetTotal += size;
			return true;
		}
		
		// Helper method to read raw data from the header buffer
		bool read_header_data(void* data, size_t size) {
			if (readHeaderOffset + size > header.size()) {
				std::cerr << "Attempt to read beyond header size.\n";
				return false;
			}
			std::memcpy(data, header.data() + readHeaderOffset, size);
			readHeaderOffset += size;
			return true;
		}
		
		// Decompress the compressed data
		bool decompress_data(const std::vector<char>& compressedData) {
			size_t readOffset = 0;
			
			// Read number of threads used during compression
			if (compressedData.size() < sizeof(unsigned int)) {
				std::cerr << "Compressed data is too small to contain thread information.\n";
				return false;
			}
			
			unsigned int compressedNumThreads = 0;
			std::memcpy(&compressedNumThreads, compressedData.data() + readOffset, sizeof(unsigned int));
			readOffset += sizeof(unsigned int);
			
			// Validate number of threads
			if (compressedNumThreads == 0) {
				std::cerr << "Invalid number of threads in compressed data: " << compressedNumThreads << "\n";
				return false;
			}
			numThreads = compressedNumThreads;
			
			// Read compressed chunk sizes
			if (compressedData.size() < readOffset + numThreads * sizeof(uLong)) {
				std::cerr << "Compressed data is too small to contain all compressed chunk sizes.\n";
				return false;
			}
			
			compressedSizes.resize(numThreads);
			for (unsigned int i = 0; i < numThreads; ++i) {
				std::memcpy(&compressedSizes[i], compressedData.data() + readOffset, sizeof(uLong));
				readOffset += sizeof(uLong);
			}
			
			// Read uncompressed chunk sizes
			if (compressedData.size() < readOffset + numThreads * sizeof(uLong)) {
				std::cerr << "Compressed data is too small to contain all uncompressed chunk sizes.\n";
				return false;
			}
			
			uncompressedSizes.resize(numThreads);
			for (unsigned int i = 0; i < numThreads; ++i) {
				std::memcpy(&uncompressedSizes[i], compressedData.data() + readOffset, sizeof(uLong));
				readOffset += sizeof(uLong);
			}
			
			// Read compressed chunks
			size_t totalCompressedChunksSize = std::accumulate(compressedSizes.begin(), compressedSizes.end(), static_cast<uLong>(0));
			if (compressedData.size() < readOffset + totalCompressedChunksSize) {
				std::cerr << "Compressed data size mismatch.\n";
				return false;
			}
			
			compressedChunks.resize(numThreads);
			for (unsigned int i = 0; i < numThreads; ++i) {
				if (compressedSizes[i] > 0) {
					compressedChunks[i].resize(compressedSizes[i]);
					std::memcpy(compressedChunks[i].data(), compressedData.data() + readOffset, compressedSizes[i]);
					readOffset += compressedSizes[i];
				}
			}
			
			// Decompress each chunk in parallel
			decompressedChunks.resize(numThreads);
			std::vector<std::future<bool>> futures;
			
			auto decompressChunk = [&](unsigned int threadIndex) -> bool {
				if (uncompressedSizes[threadIndex] == 0) {
					decompressedChunks[threadIndex].clear();
					return true;
				}
				
				decompressedChunks[threadIndex].resize(uncompressedSizes[threadIndex]);
				uLongf destLen = uncompressedSizes[threadIndex];
				int res = uncompress(reinterpret_cast<Bytef*>(decompressedChunks[threadIndex].data()), &destLen,
									 compressedChunks[threadIndex].data(), compressedSizes[threadIndex]);
				if (res != Z_OK) {
					std::cerr << "Decompression failed in thread " << threadIndex << " with error code: " << res << "\n";
					return false;
				}
				if (destLen != uncompressedSizes[threadIndex]) {
					std::cerr << "Decompressed data size mismatch in thread " << threadIndex
					<< ". Expected: " << uncompressedSizes[threadIndex]
					<< ", Got: " << destLen << "\n";
					return false;
				}
				return true;
			};
			
			for (unsigned int i = 0; i < numThreads; ++i) {
				futures.emplace_back(std::async(std::launch::async, decompressChunk, i));
			}
			
			// Wait for all decompression tasks to complete
			for (auto& fut : futures) {
				if (!fut.get()) {
					std::cerr << "One of the decompression threads failed.\n";
					return false;
				}
			}
			
			// Combine all decompressed chunks into a single buffer
			decompressedData.clear();
			decompressedData.reserve(std::accumulate(uncompressedSizes.begin(), uncompressedSizes.end(), static_cast<uLong>(0)));
			for (unsigned int i = 0; i < numThreads; ++i) {
				decompressedData.insert(decompressedData.end(),
										decompressedChunks[i].begin(),
										decompressedChunks[i].end());
			}
			
			// Initialize read offset after decompression
			readOffsetTotal = 0;
			return true;
		}
	};
};
