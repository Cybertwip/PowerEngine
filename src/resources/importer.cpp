#include <stb/stb_image.h>
#include "importer.h"
#include <filesystem>
#include "../entity/entity.h"
#include "../animation/json_animation.h"
#include "../animation/assimp_animation.h"
#include "model.h"
#include "../util/log.h"

namespace fs = std::filesystem;

namespace{

unsigned int LoadTexture(const std::string& path)
{
	std::string filename = path;
	
	anim::LOG("TextureFromFile:: " + filename);
	std::replace(filename.begin(), filename.end(), '\\', '/');
	
	unsigned int textureID;
	glGenTextures(1, &textureID);
	
	int width, height, nrComponents;
	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format = GL_RGB;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;
		
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	stbi_image_free(data);
	
	return textureID;
}

void LoadMemory(const std::shared_ptr<sfbx::Texture> texture, unsigned int *id)
{
	anim::LOG("LoadMemory");
	if (!*id)
		glGenTextures(1, id);
	glBindTexture(GL_TEXTURE_2D, *id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	unsigned char *image_data = nullptr;
	int width = 0, height = 0, components_per_pixel;
	
	image_data = stbi_load_from_memory(texture->getData().data(),
									   texture->getData().size(),
									   &width,
									   &height,
									   &components_per_pixel,
									   0);
	
	
	if (components_per_pixel == 3)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
	}
	else if (components_per_pixel == 4)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	}
	
	glGenerateMipmap(GL_TEXTURE_2D);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image_data);
}
}

namespace anim
{
Importer::Importer()
{
}

std::pair<std::shared_ptr<Model>, std::vector<std::shared_ptr<Animation>>> Importer::read_file(const char *path, LightManager& lightManager)
{
	path_ = std::string(path);
	std::vector<std::shared_ptr<Animation>> animations;
	std::shared_ptr<Model> model;
	fs::path fs_path = fs::u8path(path_);
	
	if (fs_path.extension() == ".json")
	{
		std::shared_ptr<JsonAnimation> anim = std::make_shared<JsonAnimation>(path_.c_str());
		if (anim && anim->get_type() == AnimationType::Json)
		{
			animations.emplace_back(anim);
		}
		return std::make_pair(nullptr, animations);
	}
	try
	{
		sfbx::DocumentPtr doc = sfbx::MakeDocument(path_);
		
		if(doc->valid()){
			model = import_model(doc, lightManager);
			
			animations = import_animation(doc);
			
			if (model == nullptr && animations.size() == 0)
			{
				LOG("ERROR::IMPORTER::NULL:  ANIMATIONS");
			}
			
		} else {
			LOG("ERROR::IMPORTER: INVALID FBX");
		}
		
		model->set_unit_scale(doc->global_settings.unit_scale);
	}
	catch (std::exception &e)
	{
		LOG("ERROR::IMPORTER: " + std::string(e.what()));
	}
	return std::make_pair(model, animations);
}

std::shared_ptr<Model> Importer::import_model(const sfbx::DocumentPtr doc, LightManager& lightManager)
{
	return std::make_shared<Model>(path_, doc, lightManager);
}

std::vector<std::shared_ptr<Animation>> Importer::import_animation(const sfbx::DocumentPtr doc)
{
	std::vector<std::shared_ptr<Animation>> animations;
	
	if(doc && doc->valid()){
		for(auto& stack : doc->getAnimationStacks()){
			for(auto& animationLayer : stack->getAnimationLayers()){
				animations.push_back(std::make_shared<FbxAnimation>(doc, animationLayer, path_));
			}
		}
	}
	
	return animations;
}

unsigned int Importer::LoadTextureFromFile(const char* path)
{
	std::string filename = std::string{path};
	
	return LoadTexture(filename);
}
}
