// sfbxMaterial.cpp

#include "sfbxMaterial.h"
#include "sfbxInternal.h"
#include "sfbxMaterial.h"
#include "sfbxDocument.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <sstream>

namespace {

const std::string kEmbeddedToken = "*";

std::string getFileExtension(const std::string& filename) {
	// Find the last dot in the filename
	size_t dotPos = filename.find_last_of('.');
	// Check if a dot was found and it's not the last character in the string
	if (dotPos != std::string::npos && dotPos < filename.length() - 1) {
		// Extract and return the substring after the last dot
		return filename.substr(dotPos + 1);
	} else {
		// No dot found or it's the last character, return an empty string or handle as needed
		return "";
	}
}

} // namespace

namespace sfbx {

// Video Class Implementations

ObjectClass Video::getClass() const { return ObjectClass::Video; }
ObjectSubClass Video::getSubClass() const { return ObjectSubClass::Clip; }

// Getters
bool Video::getEmbedded() const {
	return m_embedded;
}

std::string_view Video::getFilename() const {
	return m_filename;
}

const std::vector<uint8_t>& Video::getData() const {
	return m_data;
}

// Setters
void Video::setEmbedded(bool embedded) {
	m_embedded = embedded;
	// Update the FBX node property if necessary
	// Example:
	// getNode()->setProperty("Embedded", embedded);
}

void Video::setFilename(const std::string& filename) {
	m_filename = filename;
	// Update the FBX node property if necessary
	// Example:
	// getNode()->setProperty("Filename", filename);
}

void Video::setData(const std::vector<uint8_t>& data) {
	m_data = data;
	// Update the FBX node property if necessary
	// Example:
	// getNode()->setProperty("Data", data);
}

void Video::setData(std::vector<uint8_t>&& data) {
	m_data = std::move(data);
	// Update the FBX node property if necessary
	// Example:
	// getNode()->setProperty("Data", m_data);
}

void Video::importFBXObjects()
{
	super::importFBXObjects();
	// Process the properties
	std::string embeddedFilename;
	std::string imageExtension;
	
	bool isEmbedded = false;
	
	for (auto& child : getNode()->getChildren()) {
		if (child->getName() == "RelativeFilename") {
			embeddedFilename = child->getProperty(0)->getString();
		}
		if (child->getName() == "Content") {
			isEmbedded = true;
			auto content = child->getProperty(0)->getString();
			if (content.empty()) {
				isEmbedded = false;
			} else {
				m_data = std::vector<uint8_t>(content.begin(), content.end());
			}
		}
	}
	
	if (isEmbedded) {
		imageExtension = getFileExtension(embeddedFilename);
		embeddedFilename = kEmbeddedToken + ":" + std::to_string(this->getID()) + "." + imageExtension;
	}
	
	for (auto& child : getNode()->getChildren()) {
		auto stream = std::stringstream();
		
		if (child->getName() == "RelativeFilename") {
			if (isEmbedded) {
				child->getProperty(0)->assign(embeddedFilename);
			} else {
				embeddedFilename = child->getProperty(0)->getString();
			}
		}
		
		child->writeBinary(stream, 0);
		mChildStreams.push_back(std::move(stream));
	}
	
	m_embedded = isEmbedded;
	m_filename = embeddedFilename;
	
	if (!m_embedded) {
		// Open the file in binary mode
		std::filesystem::path base = std::filesystem::path(document()->global_settings.path).parent_path().make_preferred();
		std::filesystem::path relative = std::filesystem::path(m_filename).make_preferred();
		// Resolve the relative path based on the base path
		std::filesystem::path absolutePath = std::filesystem::absolute(base / relative);
		std::string result = absolutePath.string();
		
#ifdef _WIN32
		// On Windows, ensure backslashes
		std::replace(result.begin(), result.end(), '/', '\\');
#else
		// On macOS/Linux, ensure forward slashes
		std::replace(result.begin(), result.end(), '\\', '/');
#endif
		
		m_filename = result;
		
		std::ifstream file(m_filename, std::ios::binary);
		
		// Check if the file is successfully opened
		if (file.is_open()) {
			// Read the file contents into a vector<uint8_t>
			m_data = std::vector<uint8_t>(
										  (std::istreambuf_iterator<char>(file)),
										  std::istreambuf_iterator<char>()
										  );
			
			// Close the file
			file.close();
			
		} else {
			std::cerr << "Error opening file: " << m_filename << std::endl;
		}
	}
}

void Video::exportFBXObjects()
{
	super::exportFBXObjects();
	
	for (auto& stream : mChildStreams) {
		auto child = getNode()->createChild();
		child->readBinary(stream, 0);
	}
}

// Texture Class Implementations

ObjectClass Texture::getClass() const { return ObjectClass::Texture; }

// Getters
bool Texture::getEmbedded() const {
	return m_embedded;
}

std::string_view Texture::getFilename() const {
	return m_filename;
}

const std::vector<uint8_t>& Texture::getData() const {
	return m_data;
}

// Setters
void Texture::setEmbedded(bool embedded) {
	m_embedded = embedded;
	// Update the FBX node property if necessary
}

void Texture::setFilename(const std::string& filename) {
	m_filename = filename;
	// Update the FBX node property if necessary
}

void Texture::setData(const std::vector<uint8_t>& data) {
	m_data = data;
	// Update the FBX node property if necessary
}

void Texture::setData(std::vector<uint8_t>&& data) {
	m_data = std::move(data);
	// Update the FBX node property if necessary
}

void Texture::importFBXObjects()
{
	super::importFBXObjects();
	// Process the connections to find the associated Video
	Video* video = nullptr;
	for (const auto& con : document()->getConnections()) {
		if (con.dest == shared_from_this()) {
			if (con.type == Connection::Type::OO) {
				if (auto vid = sfbx::as<Video>(con.src)) {
					video = vid.get();
					break;
				}
			}
		}
	}
	
	bool hasVideo = (video != nullptr);
	
	if (hasVideo) {
		if (video->getData().empty()) {
			// Import video data
			static_cast<Object*>(video)->importFBXObjects();
		}
		
		m_embedded = video->getEmbedded();
		m_filename = video->getFilename();
		m_data = video->getData();
	} else {
		// No video connected, handle external file
		for (auto& child : getNode()->getChildren()) {
			if (child->getName() == "RelativeFilename") {
				m_filename = child->getProperty(0)->getString();
			}
			auto stream = std::stringstream();
			child->writeAscii(stream);
			mChildStreams.push_back(std::move(stream));
		}
		
		if (!m_filename.empty()) {
			std::ifstream file(m_filename, std::ios::binary);
			
			// Check if the file is successfully opened
			if (file.is_open()) {
				// Read the file contents into a vector<uint8_t>
				m_data = std::vector<uint8_t>(
											  (std::istreambuf_iterator<char>(file)),
											  std::istreambuf_iterator<char>()
											  );
				
				// Close the file
				file.close();
				
			} else {
				std::cerr << "Error opening file: " << m_filename << std::endl;
			}
		}
	}
}

void Texture::exportFBXObjects()
{
	super::exportFBXObjects();
	
	for (auto& stream : mChildStreams) {
		auto child = getNode()->createChild();
		auto streamString = stream.str();
		auto streamView = std::string_view(streamString);
		child->readAscii(streamView);
	}
}

void Texture::exportFBXConnections()
{
	// Ignore super::constructLinks()
	
	// Create OO connection to Video if exists
	for (const auto& con : document()->getConnections()) {
		if (con.src == shared_from_this() && sfbx::as<Video>(con.dest)) {
			document()->createLinkOO(shared_from_this(), con.dest);
		}
	}
	
	for (auto& parent : getParents()) {
		document()->createLinkOO(shared_from_this(), parent);
	}
}

// Material Class Implementations

ObjectClass Material::getClass() const { return ObjectClass::Material; }

// Getters
double3 Material::getAmbientColor() const
{
	return m_ambient_color;
}

double3 Material::getDiffuseColor() const
{
	return m_diffuse_color;
}

double3 Material::getSpecularColor() const
{
	return m_specular_color;
}

float64 Material::getShininess() const
{
	return m_shininess;
}

float64 Material::getOpacity() const
{
	return m_opacity;
}

std::shared_ptr<sfbx::Texture> Material::getTexture(const std::string& textureType) {
	auto it = m_textures.find(textureType);
	if (it != m_textures.end()) {
		return it->second;
	}
	return nullptr;
}

// Setters
void Material::setAmbientColor(const double3& ambient)
{
	m_ambient_color = ambient;
	// Update the FBX node property if necessary
}

void Material::setDiffuseColor(const double3& diffuse)
{
	m_diffuse_color = diffuse;
	// Update the FBX node property if necessary
}

void Material::setSpecularColor(const double3& specular)
{
	m_specular_color = specular;
	// Update the FBX node property if necessary
}

void Material::setShininess(float64 shininess)
{
	m_shininess = shininess;
	// Update the FBX node property if necessary
}

void Material::setOpacity(float64 opacity)
{
	m_opacity = opacity;
	// Update the FBX node property if necessary
}

void Material::setTexture(const std::string& textureType, const std::shared_ptr<sfbx::Texture>& texture)
{
	m_textures[textureType] = texture;
	// Update the FBX node property if necessary
}

void Material::importFBXObjects()
{
	super::importFBXObjects();
	
	// Process properties
	for (auto& child : getNode()->getChildren()) {
		if (child->getName() == "Properties70") {
			auto propertyChildren = child->getChildren();
			for (auto propertyChild : propertyChildren) {
				auto name = propertyChild->getProperty(0)->getString();
				if (name == "AmbientColor") {
					propertyChild->getPropertiesValues<double3>(4, m_ambient_color);
				}
				if (name == "DiffuseColor") {
					propertyChild->getPropertiesValues<double3>(4, m_diffuse_color);
				}
				if (name == "SpecularColor") {
					propertyChild->getPropertiesValues<double3>(4, m_specular_color);
				}
				if (name == "Shininess") {
					m_shininess = propertyChild->getProperty(4)->getValue<float64>();
				}
				if (name == "Opacity") {
					m_opacity = propertyChild->getProperty(4)->getValue<float64>();
				}
			}
		}
		auto stream = std::stringstream();
		child->writeAscii(stream);
		mChildStreams.push_back(std::move(stream));
	}
	
	// Process connections to get textures
	for (const auto& con : document()->getConnections()) {
		if (con.dest == shared_from_this()) {
			if (con.type == Connection::Type::OP) {
				if (auto texture = sfbx::as<Texture>(con.src)) {
					m_textures[con.property_name] = texture;
				}
			}
		}
	}
}

void Material::exportFBXObjects()
{
	super::exportFBXObjects();
	
	for (auto& stream : mChildStreams) {
		auto child = getNode()->createChild();
		auto streamString = stream.str();
		auto streamView = std::string_view(streamString);
		child->readAscii(streamView);
	}
}

void Material::exportFBXConnections()
{
	for (auto& parent : getParents()) {
		document()->createLinkOO(shared_from_this(), parent);
	}
	
	for (const auto& texPair : m_textures) {
		document()->createLinkOP(texPair.second, shared_from_this(), texPair.first);
	}
}

// Implementation Class Implementations

ObjectClass Implementation::getClass() const { return ObjectClass::Implementation; }

// BindingTable Class Implementations

ObjectClass BindingTable::getClass() const { return ObjectClass::BindingTable; }

} // namespace sfbx
