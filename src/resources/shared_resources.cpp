#include "shared_resources.h"

#include "../animation/animator.h"
#include "../animation/animation.h"
#include "../graphics/shader.h"
#include "../graphics/opengl/gl_shader.h"
#include "../entity/entity.h"
#include "model.h"
#include "importer.h"
#include "exporter.h"
#include "../entity/components/component.h"
#include "../entity/components/pose_component.h"
#include "../entity/components/serialization_component.h"
#include "../entity/components/renderable/mesh_component.h"
#include "../entity/components/renderable/armature_component.h"
#include "../entity/components/renderable/physics_component.h"
#include "../entity/components/renderable/light_component.h"
#include "../entity/components/animation_component.h"
#include "../entity/components/transform_component.h"
#include "physics/PhysicsUtils.h"

#include "scene/scene.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../graphics/opengl/gl_mesh.h"
#include "../graphics/post_processing.h"

#include "shading/light_manager.h"

#include "utility.h"

#ifdef __APPLE__
#include <unistd.h>
#endif

#include "stb/stb_image_write.h"
#include <random>
#include <algorithm>

namespace {
typedef std::int16_t mGuiID;

mGuiID HashStr(const std::string& data)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<std::int16_t> dis;
	
	std::int16_t seed = dis(gen);
	std::int16_t hash = ~seed;
	
	if (!data.empty())
	{
		for (char c : data)
		{
			if (c == '#' && data.size() >= 2 && data[0] == '#' && data[1] == '#')
				hash = seed;
			hash = (hash >> 8) ^ ((hash & 0xFF) ^ c);
		}
	}
	
	return ~hash;
}

std::string find_ffmpeg_command(const std::string& export_path) {
#ifdef __APPLE__
	// Define the location of ffmpeg for Apple platforms
	std::string ffmpeg_location = "/opt/homebrew/bin/ffmpeg";
	
	// Check if ffmpeg executable exists at the specified location
	if (access(ffmpeg_location.c_str(), X_OK) == 0) {
		// Construct the ffmpeg command using the provided export path
		std::string ffmpeg_command = ffmpeg_location + " -y -framerate 60 -i " + export_path + "/project%d.png -c:v libx264 -crf 18 -r 60 -pix_fmt yuv420p -vf \"scale=-1:1080:flags=lanczos\" " + export_path + "/project.mp4";
		return ffmpeg_command;
	}
#else
	// For non-Apple platforms, search for ffmpeg in the PATH
	// Implement the PATH searching logic here (as shown in the previous response)
#endif
	
	// Return an empty string if ffmpeg executable is not found
	return "";
}

void applyBoxBlurToPixel(std::vector<unsigned char>& pixels, int x, int y, int width, int height) {
	if(x <= 0 || x >= width - 1 || y <= 0 || y >= height - 1) return; // Skip edge pixels
	
	int numChannels = 4; // Assuming RGBA
	int idx = (y * width + x) * numChannels;
	std::vector<int> sum(4, 0); // To store the sum of RGBA values
	
	// Average with the immediate neighbors
	for(int dy = -1; dy <= 1; ++dy) {
		for(int dx = -1; dx <= 1; ++dx) {
			int neighborIdx = ((y + dy) * width + (x + dx)) * numChannels;
			for(int channel = 0; channel < 4; ++channel) {
				sum[channel] += pixels[neighborIdx + channel];
			}
		}
	}
	
	// Apply averaged color; divide by 9 to average the sum
	for(int channel = 0; channel < 4; ++channel) {
		pixels[idx + channel] = sum[channel] / 9;
	}
}

void takeScreenshot(int width, int height, GLuint textureID, const std::string& filename) {
	// Allocate memory for pixel data
	std::vector<unsigned char> pixels(4 * width * height); // 4 channels (RGBA) per pixel
	
	// Bind the framebuffer's screen texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	
	// Read pixel data from the texture
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	
	// Reverse the rows because OpenGL's origin is at the bottom-left, while most image formats have the origin at the top-left
	std::vector<unsigned char> flippedPixels(4 * width * height);
	for (int row = 0; row < height; ++row) {
		std::memcpy(flippedPixels.data() + (height - row - 1) * width * 4, pixels.data() + row * width * 4, width * 4);
	}
	
	// Write pixel data to an image file using STB library
	stbi_write_png(filename.c_str(), width, height, 4, flippedPixels.data(), 0);
}

void RecursivelyAddDirectoryNodes(DirectoryNode& parentNode, std::filesystem::directory_iterator directoryIterator)
{
	for (const std::filesystem::directory_entry& entry : directoryIterator)
	{
		
		if(entry.is_directory()){
			DirectoryNode& childNode = parentNode.Children.emplace_back();
			childNode.FullPath = entry.path().u8string();
			childNode.FileName = entry.path().filename().u8string();
			if (childNode.IsDirectory = entry.is_directory(); childNode.IsDirectory){
				RecursivelyAddDirectoryNodes(childNode, std::filesystem::directory_iterator(entry));
			}
		} else {
			if (entry.is_regular_file() && (entry.path().extension() == ".fbx" || entry.path().extension() == ".seq" ||
											entry.path().extension() == ".pwr" || entry.path().extension() == ".cmp")) {
				DirectoryNode& childNode = parentNode.Children.emplace_back();
				childNode.FullPath = entry.path().u8string();
				childNode.FileName = entry.path().filename().u8string();
			}
			
		}
	}
	
	auto moveDirectoriesToFront = [](const DirectoryNode& a, const DirectoryNode& b) { return (a.IsDirectory > b.IsDirectory); };
	std::sort(parentNode.Children.begin(), parentNode.Children.end(), moveDirectoriesToFront);
}

DirectoryNode CreateDirectoryNodeTreeFromPath(const std::filesystem::path& rootPath)
{
	if (!std::filesystem::exists(rootPath) || !std::filesystem::is_directory(rootPath)) {
		std::filesystem::create_directories(rootPath);
	}
	
	DirectoryNode rootNode;
	rootNode.FullPath = rootPath.u8string();
	rootNode.FileName = rootPath.filename().u8string();
	if (rootNode.IsDirectory = std::filesystem::is_directory(rootPath); rootNode.IsDirectory)
		RecursivelyAddDirectoryNodes(rootNode, std::filesystem::directory_iterator(rootPath));
	
	return rootNode;
}

void RecursivelyCleanDirectoryNodes(DirectoryNode& parentNode)
{
	auto it = std::remove_if(parentNode.Children.begin(), parentNode.Children.end(),
							 [](DirectoryNode& childNode)
							 {
		
		if(!childNode.IsDirectory){
			return false;
		}
		
		bool hasFile = false;
		
		for(auto& entry : childNode.Children){
			if(!entry.IsDirectory){
				hasFile = true;
			}
		}
		
		if (childNode.Children.empty() || !hasFile)
		{
			return true; // Mark for removal
		}
		else
		{
			RecursivelyCleanDirectoryNodes(childNode);
			return false;
		}
	});
	
	parentNode.Children.erase(it, parentNode.Children.end());
}


void RecursivelyImportModels(anim::SharedResources& resources, DirectoryNode& parentNode)
{
	if(!parentNode.IsDirectory && parentNode.FileName.find(".fbx") != std::string::npos){
		resources.import_model(parentNode.FullPath.c_str());
	} else {
		for(auto& childNode : parentNode.Children){
			RecursivelyImportModels(resources, childNode);
		}
	}
}


static DirectoryNode rootNode;
}

namespace {

std::string get_home_path(){
#if __APPLE__
	std::string user_name;
	user_name = "/Users/" + std::string(getenv("USER"));
	return user_name + "/Documents/" + "PowerLibrary/PowerProject/Content";
#endif
}

}

namespace anim
{
SharedResources::SharedResources(Scene& scene, const std::string& defaultScenePath) : _scene(scene), _defaultScenePath(defaultScenePath)
{
}
SharedResources::~SharedResources()
{
	LOG("~SharedResources");
	glDeleteBuffers(1, &matrices_UBO_);
}

void SharedResources::initialize(){
	mPostProcessing.reset(new PostProcessing());
	
	init_animator();
	init_shader();
	root_entity_ = std::make_shared<Entity>(shared_from_this(), "Scene", single_entity_list_.size());
	root_entity_->set_root(root_entity_);
	single_entity_list_.push_back(root_entity_);
	
	ArmatureComponent::setShape(gl::CreateBiPyramid());

	std::filesystem::current_path(DEFAULT_CWD);
	
	auto homePath = std::filesystem::current_path();
	
	rootNode = CreateDirectoryNodeTreeFromPath(homePath);
	
	RecursivelyCleanDirectoryNodes(rootNode);
	RecursivelyImportModels(*this, rootNode);
}

void SharedResources::refresh_directory_node(){
	auto homePath = std::filesystem::current_path();
	
	rootNode = CreateDirectoryNodeTreeFromPath(homePath);
	RecursivelyCleanDirectoryNodes(rootNode);
}

Animator *SharedResources::get_mutable_animator()
{
	return animator_.get();
}

std::shared_ptr<Shader> SharedResources::get_mutable_shader(const std::string &name)
{
	return shaders_[name];
}

std::shared_ptr<Model> SharedResources::import_model(const char *path, float scale)
{
	auto it = std::find_if(_model_cache.begin(), _model_cache.end(), [path](auto& cached){
		return cached.first == path;
	});
	
	if(it == _model_cache.end()){
		Importer import{};
		import.mScale = scale;
		auto [model, animations] = import.read_file(path);
		
		add_animation_set(path, model, animations);
		
		add_animations(animations);
		
		_model_cache[path] = model;
		
		return model;
	} else {
		return _model_cache[path];
	}
}
//
//void SharedResources::import_animation(const char *path, float scale)
//{
//	Importer import{};
//	import.mScale = scale;
//	auto [_, animations] = import.read_file(path);
//
//	add_animations(animations);
////	add_entity(model, path);
//}

void SharedResources::export_animation(std::shared_ptr<anim::Entity> entity, const char *save_path, bool is_linear)
{
	if (entity && entity->get_mutable_root())
	{
		Exporter exporter;
		exporter.is_linear_ = is_linear;
		//exporter.to_glft2(entity->get_mutable_root(), save_path, model_path_[entity->get_mutable_root()->get_id()].c_str());
	}
}

void SharedResources::serialize(const std::string& path){
	std::string serializationPath = path;
	if(path.empty()){
		serializationPath = _defaultScenePath;
	}
	
	Json::Value root;  // 'root' will hold the entire JSON object.
	Json::Value cameras(Json::arrayValue); // Creates a JSON array to store your models.
	Json::Value lights(Json::arrayValue); // Creates a JSON array to store your models.
	Json::Value models(Json::arrayValue); // Creates a JSON array to store your models.
	
	for (const auto& entity : _scene.get_cameras()) {
		Json::Value data; // Create a JSON object for this camera.
		data["id"] = entity->get_id(); // Assuming each camera has an 'Id' field.
		
		const glm::mat4& transform = entity->get_local();
		Json::Value transformArray(Json::arrayValue);
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				transformArray.append(transform[i][j]);
			}
		}
		data["transform"] = transformArray;
		
		cameras.append(data); // Add the camera object to the cameras array.
	}
	
	
	for (const auto& entity : _scene.get_lights()) {
		Json::Value data; // Create a JSON object for this camera.
		data["id"] = entity->get_id(); // Assuming each camera has an 'Id' field.
		
		const glm::mat4& transform = entity->get_local();
		Json::Value transformArray(Json::arrayValue);
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				transformArray.append(transform[i][j]);
			}
		}
		data["transform"] = transformArray;

		lights.append(data);
	}
	
	// Assuming the rest of your serialization logic integrates the 'cameras' Json::Value into your root object appropriately.
	
	animator_->set_current_time(0);
	//	animator_->update_sequencer(_scene, shared_from_this(), ); //@TODO
	
	// Populate the JSON array with model paths and IDs.
	
	auto children = root_entity_->get_children_recursive();
	for (const auto child : children) {
		
		if(auto camera = child->get_component<CameraComponent>(); camera){
			continue;
		}
		if(auto light = child->get_component<DirectionalLightComponent>(); light){
			continue;
		}
				
		auto serialization = child->get_component<ModelSerializationComponent>();
		
		if(serialization){
			Json::Value modelData; // Create a JSON object for this model.
			modelData["id"] = child->get_id(); // Assuming each model has an 'Id' field.
			
			const glm::mat4& transform = child->get_local();
			Json::Value transformArray(Json::arrayValue);
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					transformArray.append(transform[i][j]);
				}
			}
			modelData["transform"] = transformArray;

			modelData["path"] = std::filesystem::relative(serialization->get_model_path(), DEFAULT_CWD).string(); // Convert path to relative and store it.
			
			models.append(modelData); // Add the model object to the models array.
		}
		
	}
	
	// Add the array to the root object with a key named "models".
	root["cameras"] = cameras;
	root["lights"] = lights;
	root["models"] = models;
	
	// Write the JSON object to a file.
	std::ofstream file(serializationPath);
	if (file.is_open()){
		Json::StreamWriterBuilder builder;
		std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
		writer->write(root, &file);
		file.close();
	}
}

void SharedResources::deserialize(const std::string& path){
	std::string serializationPath = path;
	if(path.empty()){
		serializationPath = _defaultScenePath;
	} else {
		_defaultScenePath = serializationPath;
	}
	
	Json::Value root;   // This will hold the root value after parsing.
	std::ifstream file(serializationPath, std::ifstream::binary); // Open the file in binary mode to avoid issues with end-of-line characters.
	file >> root;  // Parse the JSON from the file into the root Json::Value.
	
	if (root.isNull() || root.empty()) {
		std::cerr << "Warning: File is empty or content is not valid JSON: " << serializationPath << std::endl;
		
		LightManager::getInstance()->clearLights();

		single_entity_list_.clear();
		animations_.clear();
		animationSets_.clear();
		_model_cache.clear();
				
		root_entity_ = std::make_shared<Entity>(shared_from_this(), "Scene", single_entity_list_.size());
		root_entity_->set_root(root_entity_);
		single_entity_list_.push_back(root_entity_);
		_scene.clear_cameras();
		_scene.clear_lights();

		auto camera = create_camera();
		auto light = create_light();
		
		
		auto homePath = std::filesystem::current_path();
		
		rootNode = CreateDirectoryNodeTreeFromPath(homePath);
		
		RecursivelyCleanDirectoryNodes(rootNode);
		RecursivelyImportModels(*this, rootNode);
		
		return;
	}
	
	single_entity_list_.clear();
	animations_.clear();
	animationSets_.clear();
	
	_model_cache.clear();
	
	_scene.clear_cameras();
	_scene.clear_lights();
	
	LightManager::getInstance()->clearLights();
	
	root_entity_ = std::make_shared<Entity>(shared_from_this(), "Scene", single_entity_list_.size());
	root_entity_->set_root(root_entity_);
	single_entity_list_.push_back(root_entity_);
	
	// Extract the model paths from the JSON array and add them to the vector.
	
	const Json::Value cameras = root["cameras"];
	
	for (const auto& data : cameras){
		// Assuming you have a method to find or create a camera by its ID
		int cameraId = data["id"].asInt();
		
		glm::mat4 transform(1.0f); // Default to identity matrix
		if (data.isMember("transform") && data["transform"].isArray()) {
			for (int i = 0; i < 16; ++i) {
				// Assuming the transform array is flat (16 elements)
				*(glm::value_ptr(transform) + i) = data["transform"][i].asFloat();
			}
		}
		
		auto entity = create_camera();
		entity->set_local(transform);
	}
	
	const Json::Value lights = root["lights"];
	
	for (const auto& data : lights){
		// Assuming you have a method to find or create a camera by its ID
		int lightId = data["id"].asInt();
		
		glm::mat4 transform(1.0f); // Default to identity matrix
		if (data.isMember("transform") && data["transform"].isArray()) {
			for (int i = 0; i < 16; ++i) {
				// Assuming the transform array is flat (16 elements)
				*(glm::value_ptr(transform) + i) = data["transform"][i].asFloat();
			}
		}
		
		// ? parameters
		auto entity = create_light();
		entity->set_local(transform);
	}

	
	if(_scene.get_cameras().empty()){
		create_camera();
	}
	
	const Json::Value models = root["models"];
	for (const auto& modelData : models){
		auto modelPath = std::filesystem::absolute(modelData["path"].asString()).string();
		
		auto model = import_model(modelPath.c_str());
		auto entity = parse_model(model, modelPath.c_str());
		
		
		// Deserialize the transform
		glm::mat4 transform(1.0f); // Default to identity matrix
		if (modelData.isMember("transform") && modelData["transform"].isArray()) {
			for (int i = 0; i < 16; ++i) {
				// Assuming the transform array is flat (16 elements)
				*(glm::value_ptr(transform) + i) = modelData["transform"][i].asFloat();
			}
		}
		
		// Apply the deserialized transformation to your entity
		// This assumes you have a method to set the transformation of an entity
		entity->set_local(transform);
		
		auto root = get_root_entity();
		
		std::vector<std::string> existingNames; // Example existing actor names
		std::string prefix = "Actor"; // Prefix for actor names
		
		auto children = root->get_mutable_children();
		
		// Example iteration to get existing names (replace this with your actual iteration logic)
		for (int i = 0; i < children.size(); ++i) {
			std::string actorName = children[i]->get_name();
			existingNames.push_back(actorName);
		}
		
		std::string uniqueActorName = GenerateUniqueActorName(existingNames, prefix);
		
		entity->set_name(uniqueActorName);

		root->add_children(entity);
	}
	
	
	auto homePath = std::filesystem::current_path();
	
	rootNode = CreateDirectoryNodeTreeFromPath(homePath);
	
	RecursivelyCleanDirectoryNodes(rootNode);
	RecursivelyImportModels(*this, rootNode);

}

std::shared_ptr<Entity> SharedResources::parse_model(std::shared_ptr<Model> &model, const char *path, bool serialize)
{
	if (!model)
	{
		return;
	}
	
	std::shared_ptr<Entity> entity = nullptr;
	convert_to_entity(entity, model, model->get_root_node(), nullptr, 0, nullptr);

	if(serialize){
		auto serialization = entity->add_component<ModelSerializationComponent>();
		
		serialization->set_model_path(path);
		
		entity->get_animation_tracks()->mType = TrackType::Actor;
	} else {
		entity->get_animation_tracks()->mType = TrackType::Other;
	}
	
	auto childrenRecursive = entity->get_children_recursive();
	
	auto mesh = entity->get_component<MeshComponent>();
	
	for(auto& child : childrenRecursive){
		auto mesh = child->get_component<MeshComponent>();
		
		if(mesh){
			mesh->selectionColor = entity->get_id() << 8;
			mesh->set_unit_scale(model->get_unit_scale());
		}
	}
	
	
	if(entity && animations_.size() > 0){
		auto pose = entity->get_mutable_root()->get_component<PoseComponent>();
				
		animator_->set_end_time(animations_.back()->get_duration());
		
	}
	if (entity)
	{
		entity->set_name(model->get_name());
		
//		auto [t, r, s] = DecomposeTransform(entity->get_local());
//		
//		s *= model->get_unit_scale(); // Normalized when imported
//		
//		auto transformation = ComposeTransform(t, r, s);
//		
//		entity->set_local(transformation);
	}
	
	return entity;
}
void SharedResources::add_animations(const std::vector<std::shared_ptr<Animation>> &animations)
{
	for (auto &animation : animations)
	{
		add_animation(animation);
	}
	LOG("Animation Add: " + std::to_string(animations_.size()));
}

void SharedResources::add_animation_set(const std::string& path, std::shared_ptr<Model> model, const std::vector<std::shared_ptr<Animation>> &animations){
	animationSets_[path] = {model, animations};
}

AnimationSet& SharedResources::getAnimationSet(const std::string& path){
	return animationSets_[path];
}

const std::map<std::string, AnimationSet>& SharedResources::getAnimationSets(){
	return animationSets_;
}

void SharedResources::add_animation(std::shared_ptr<Animation> animation)
{
	if (animation)
	{
		if(animations_.empty()){
			animations_.push_back(animation);
			animations_.back()->set_id(static_cast<int>(animations_.size() - 1));
		} else {
			
			bool existing = false;
			
			for(auto& animationPointer : animations_){
				if(animationPointer->get_id() == animation->get_id()
				   || animationPointer->get_path() == animation->get_path()){
					
					if(animationPointer->get_id() != animation->get_id()){
						animation->set_id(animationPointer->get_id());
					}
					existing = true;
					break;
				}
			}
			
			if(!existing){
				animation->set_id(animations_.back()->get_id() + 1);
				animations_.push_back(animation);
			}
		}
	}
}
void SharedResources::add_shader(const std::string &name, const char *vs_path, const char *fs_path)
{
	shaders_[name] = std::make_shared<gl::GLShader>(vs_path, fs_path);
}
void SharedResources::update(ui::UiContext& ui_context, bool includeSequencer)
{
	if(includeSequencer){
		animator_->update_sequencer(_scene, ui_context);
	}
	_scene.get_physics_world().update(0.016f);

	if(root_entity_){
		root_entity_->update();
	}
}
}

namespace anim
{
void SharedResources::init_animator()
{
	animator_ = std::make_unique<Animator>(shared_from_this());
}
void SharedResources::init_shader()
{
	add_shader("armature", "./resources/shaders/armature.vs", "./resources/shaders/phong_model.fs");
	add_shader("model", "./resources/shaders/simple_model.vs", "./resources/shaders/phong_model.fs");
	add_shader("animation", "./resources/shaders/animation_skinning.vs", "./resources/shaders/phong_model.fs");
	
	add_shader("framebuffer", "./resources/shaders/simple_framebuffer.vs", "./resources/shaders/simple_framebuffer.fs");
	add_shader("grid", "./resources/shaders/grid.vs", "./resources/shaders/grid.fs");
	add_shader("outline", "./resources/shaders/simple_framebuffer.vs", "./resources/shaders/outline.fs");
	
	add_shader("shadow", "./resources/shaders/shadow.vs", "./resources/shaders/shadow.fs");
	add_shader("ui", "./resources/shaders/ui_model.vs", "./resources/shaders/ui_model.fs");

	mPostProcessing->set_shaders(shaders_["framebuffer"].get(), shaders_["outline"].get());
	
	auto model_id = get_mutable_shader("model")->get_id();
	auto animation_id = get_mutable_shader("animation")->get_id();
	auto armature_id = get_mutable_shader("armature")->get_id();
	auto grid_id = get_mutable_shader("grid")->get_id();
	
	unsigned int uniform_block_id_model = glGetUniformBlockIndex(model_id, "Matrices");
	unsigned int uniform_block_id_animation = glGetUniformBlockIndex(animation_id, "Matrices");
	unsigned int uniform_block_id_armature = glGetUniformBlockIndex(armature_id, "Matrices");
	unsigned int uniform_block_id_grid = glGetUniformBlockIndex(grid_id, "Matrices");
	
	glUniformBlockBinding(model_id, uniform_block_id_model, 0);
	glUniformBlockBinding(animation_id, uniform_block_id_animation, 0);
	glUniformBlockBinding(armature_id, uniform_block_id_armature, 0);
	glUniformBlockBinding(grid_id, uniform_block_id_grid, 0);
	
	glGenBuffers(1, &matrices_UBO_);
	glBindBuffer(GL_UNIFORM_BUFFER, matrices_UBO_);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4) + sizeof(glm::vec4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, matrices_UBO_, 0, 2 * sizeof(glm::mat4) + sizeof(glm::vec4));
}
void SharedResources::set_ubo_position(const glm::vec3 &position)
{
	glm::vec4 paddedPosition = glm::vec4(position, 1.0f); // Pad position for alignment
	glBindBuffer(GL_UNIFORM_BUFFER, matrices_UBO_);
	glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::vec4), glm::value_ptr(paddedPosition));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
void SharedResources::set_ubo_projection(const glm::mat4 &projection)
{
	glBindBuffer(GL_UNIFORM_BUFFER, matrices_UBO_);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
void SharedResources::set_ubo_view(const glm::mat4 &view)
{
	glBindBuffer(GL_UNIFORM_BUFFER, matrices_UBO_);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
void SharedResources::set_dt(float dt)
{
	dt_ = dt;
	animator_->update(dt);
}

std::shared_ptr<Entity> SharedResources::get_root_entity()
{
	return root_entity_;
}

void SharedResources::set_root_entity(std::shared_ptr<Entity> entity){
	root_entity_ = entity;
}

std::vector<std::shared_ptr<Animation>> &SharedResources::get_animations()
{
	return animations_;
}
}

namespace anim
{
void SharedResources::convert_to_entity(std::shared_ptr<Entity> &entity,
										std::shared_ptr<Model> &model,
										const std::shared_ptr<ModelNode> &model_node,
										std::shared_ptr<Entity> parent_entity, int child_num, std::shared_ptr<Entity> root_entity)
{
	const std::string &name = model_node->name;
	LOG("- - TO ENTITY: " + name);
	
	int32_t entity_id = single_entity_list_.size();
	
	for(auto entity : single_entity_list_){
		if(entity->get_id() >= entity_id){
			entity_id = entity->get_id();
		}
	}
	
	entity_id++;
	
		
	//	std::string hash = name + std::to_string(entity_id);
	//	auto hashId = ImHashStr(hash.c_str());
	//
	//	entity_id = hashId;
	
	entity.reset(new Entity(shared_from_this(), name, entity_id, parent_entity));
	
	single_entity_list_.push_back(entity);

	if (!root_entity)
	{
		root_entity = entity;
		entity->set_parent(entity);
	}
	entity->set_root(root_entity);
	
	auto bind_pose_transformation = model_node->relative_transformation;
	
	// mesh component
	if (model_node->meshes.size() != 0)
	{
		auto mesh = entity->add_component<MeshComponent>();
		
		mesh->set_bindpose(bind_pose_transformation);
		
		mesh->set_meshes(model_node->meshes);
		mesh->set_shader(shaders_["model"].get());
		mesh->isDynamic = false;
		LOG("===================== MESH: " + name);
		if (model_node->has_bone)
		{
			mesh->set_shader(shaders_["animation"].get());
			mesh->isDynamic = true;
		}
		
		mesh->uniqueIdentifier = entity_id << 8;
	}
		
	// find mising bone
	auto parent_armature = (parent_entity) ? parent_entity->get_component<ArmatureComponent>() : nullptr;
	auto bone_info = model->get_pointer_bone_info(name);
	//	entity->set_local(bind_pose_transformation);
	
	std::shared_ptr<PoseComponent> parent_pose = (parent_armature) ? parent_armature->get_pose() : nullptr;
	if (parent_armature && !bone_info)
	{
		auto parent_offset = parent_armature->get_bone_offset();
		BoneInfo missing_bone_info{
			model->bone_count_++,
			glm::inverse(glm::inverse(parent_offset) * bind_pose_transformation),
		};
		parent_pose->add_bone(name, missing_bone_info);
		model->bone_info_map_[name] = missing_bone_info;
		bone_info = &(model->bone_info_map_[name]);
	}
	
	// armature component
	if (bone_info)
	{
		std::shared_ptr<PoseComponent> pose = parent_pose;
		auto armature = entity->add_component<ArmatureComponent>();
	
		
		armature->uniqueIdentifier = entity_id << 8;
		
		glm::mat4 parent_offset = glm::identity<glm::mat4>();
		if(parent_armature){
			parent_offset = parent_armature->get_bone_offset();
		}
		
		if (parent_armature)
		{
			glm::vec4 relative_pos = bind_pose_transformation * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			pose = parent_armature->get_pose();
			parent_armature->set_local_scale(child_num, glm::distance(relative_pos, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
			parent_armature->set_local_rotation_with_target(child_num, glm::vec3(relative_pos));
			armature->set_local_scale(0, parent_armature->get_local_scale());
			armature->set_local_rotation(0, parent_armature->get_local_rotation());
		}
		else
		{
			auto animation = root_entity->add_component<AnimationComponent>();
			if (animations_.size() > 0)
			{
				animation->set_animation(animations_.back());
				
				animations_.back()->set_owner(root_entity);
			}
			pose = root_entity->add_component<PoseComponent>();
			LOG("=============== POSE: " + parent_entity->get_name());
			
			animation->set_root_bone_name(parent_entity->get_name());
			
			pose->set_bone_info_map(model->bone_info_map_);
			pose->set_animator(animator_.get());
			pose->set_shader(shaders_["animation"].get());
			pose->set_animation_component(animation);
			pose->set_armature_root(entity);
			armature->set_local_scale(0, 100.0f);
		}
		
		armature->set_pose(pose);
		armature->set_name(name);
		armature->set_bone_id(bone_info->id);
		//		armature->set_bone_offset(glm::inverse(glm::inverse(parent_offset) * bind_pose_transformation));
		armature->set_bone_offset(glm::inverse(glm::inverse(parent_offset) * bind_pose_transformation));
		armature->set_bind_pose(bind_pose_transformation);
		armature->set_shader(shaders_["armature"].get());
	}
	int child_size = model_node->childrens.size();
	auto &children = entity->get_mutable_children();
	children.resize(child_size);
	
	for (int i = 0; i < child_size; i++)
	{
		LOG("---------------------------------------------------------- Children: " + model_node->childrens[i]->name);
		convert_to_entity(children[i], model, model_node->childrens[i], entity, i, root_entity);
	}
}

std::shared_ptr<Entity> SharedResources::get_entity(const std::string& name)
{
	auto it = std::find_if(single_entity_list_.begin(), single_entity_list_.end(),
						   [&name](const std::shared_ptr<Entity>& entity) {
		return entity->get_name() == name;
	});
	
	if (it != single_entity_list_.end()) {
		// Entity with the specified ID found
		return *it;
	} else {
		for(auto& entity : single_entity_list_){
			auto children = entity->get_children_recursive();
			for(auto child : children){
				if(child->get_name() == name){
					return child;
				}
			}
		}
	}
	
	// Entity with the specified ID not found
	return nullptr;
}


std::shared_ptr<Entity> SharedResources::get_entity(int id)
{
	auto it = std::find_if(single_entity_list_.begin(), single_entity_list_.end(),
						   [id](const std::shared_ptr<Entity>& entity) {
		return entity->get_id() == id;
	});
	
	if (it != single_entity_list_.end()) {
		// Entity with the specified ID found
		return *it;
	} else {
		for(auto& entity : single_entity_list_){
			auto children = entity->get_children_recursive();
			for(auto child : children){
				if(child->get_id() == id){
					return child;
				}
			}
		}
	}
	
	// Entity with the specified ID not found
	return nullptr;
}

void SharedResources::remove_entity(int id)
{
	if(id == -1){
		return;
	}
	
	auto itCam = std::find_if(_scene.get_cameras().begin(), _scene.get_cameras().end(),
							  [id](const std::shared_ptr<Entity>& entity) {
		return entity->get_id() == id;
	});
	
	
	if(itCam != _scene.get_cameras().end()){
		_scene.remove_camera(*itCam);
	}
	
	auto itLight = std::find_if(_scene.get_lights().begin(), _scene.get_lights().end(),
							  [id](const std::shared_ptr<Entity>& entity) {
		return entity->get_id() == id;
	});
	
	
	if(itLight != _scene.get_lights().end()){
		_scene.remove_light(*itLight);
	}
	
	auto it = std::find_if(single_entity_list_.begin(), single_entity_list_.end(),
							  [id](const std::shared_ptr<Entity>& entity) {
		return entity->get_id() == id;
	});

	
	if (it != single_entity_list_.end()) {
		if(root_entity_){
			root_entity_->removeChild(*it);
		}
		single_entity_list_.erase(it);
	}
}

std::shared_ptr<Animation> SharedResources::get_animation(int id)
{
	auto it = std::find_if(animations_.begin(), animations_.end(), [id](std::shared_ptr<Animation> pointer){
		return pointer->get_id() == id;
	});
	
	if(it != animations_.end()){
		return (*it);
	} else {
		return nullptr;
	}
}

void SharedResources::start_export_sequence(const std::string& filename){
	_export_path = filename;
	_exporting_sequence = true;
	_export_frame = 0;
	_frame_exported = false;
	
	animator_->set_is_export(true);
}

void SharedResources::offload_screen(int width, int height, unsigned int textureId){
	if(_exporting_sequence){
		if(!_frame_exported){
			_frame_exported = true;
			takeScreenshot(width, height, textureId, _export_path + "/project" + std::to_string(_export_frame) + ".png");
		}
		
		if(_export_frame < animator_->get_current_frame_time()){
			_export_frame = static_cast<int64_t>(animator_->get_current_frame_time());
			_frame_exported = false;
		}
	}
}

void SharedResources::end_export_sequence(bool isCancel){
	_exporting_sequence = false;
	
	animator_->set_is_export(false);
	
	if(!isCancel){
		// Create the ffmpeg command to generate the video
		std::string ffmpeg_command = find_ffmpeg_command(_export_path);
		
		// Execute the ffmpeg command
		int result = system(ffmpeg_command.c_str());
		if (result != 0) {
			// Handle error if ffmpeg command execution fails
			// You can log an error message or take appropriate action
			// For example:
			std::cerr << "Error: Failed to execute ffmpeg command" << std::endl;
		} else {
#ifdef __APPLE__
			auto openFolder = "open " + _export_path;
			auto openMovie = "open " + _export_path + "/project.mp4";
			system(openFolder.c_str());
			system(openMovie.c_str());
#endif
		}
	}
	
	// Remove all remaining PNG files in the _export_path directory
	for (const auto& entry : std::filesystem::directory_iterator(_export_path)) {
		if (entry.path().extension() == ".png") {
			std::filesystem::remove(entry.path());
		}
	}
}

std::shared_ptr<anim::Entity> SharedResources::create_camera(const glm::vec3& position){
	auto root = get_root_entity();
	
	std::vector<std::string> existingNames; // Example existing actor names
	std::string prefix = "Camera"; // Prefix for actor names
	
	auto children = root->get_mutable_children();
	
	// Example iteration to get existing names (replace this with your actual iteration logic)
	for (int i = 0; i < children.size(); ++i) {
		std::string actorName = children[i]->get_name();
		existingNames.push_back(actorName);
	}
	
	std::string uniqueActorName = anim::GenerateUniqueActorName(existingNames, prefix);

	auto modelPath = std::filesystem::path(DEFAULT_CWD) / "Models/Camera.ubx";
	
	auto model = import_model(modelPath.string().c_str());
	auto camera = parse_model(model, modelPath.string().c_str(), false);
	
	auto default_camera = std::dynamic_pointer_cast<CameraComponent>(camera->add_component<CameraComponent>());
	
	default_camera->initialize(camera, shared_from_this());
	
	glm::vec3 direction;
	
	// Check if position is not a zero vector before normalizing
	if (position != glm::vec3(0, 0, 0)) {
		direction = glm::normalize(-position); // Assuming you want the direction from position towards the origin
	} else {
		// Handle the case where position is a zero vector (e.g., set a default direction)
		direction = glm::vec3(1, 0, 0); // Example: default direction along the x-axis
	}
	
	float yaw = atan2(-direction.x, -direction.z); // Note the inversion to point towards the origin

	glm::quat yaw_quaternion = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));

	auto component = camera->get_component<TransformComponent>();
	
	auto [t, r, s] = DecomposeTransform(component->get_mat4());
	
	t = position;
	r = yaw_quaternion;
	
	auto transform = ComposeTransform(t, r, s);
	
	camera->set_local(transform);
	
	default_camera->update_camera_vectors();
	
	camera->set_name(uniqueActorName);
	
	if(root){
		root->add_children(camera);
	}
	
	auto ui_shader = get_mutable_shader("ui");

	auto meshes = camera->get_children_recursive();
	for(auto child : meshes){
		if(auto mesh = child->get_component<MeshComponent>(); mesh){
			mesh->isInterface = true;
			mesh->set_shader(ui_shader.get());
		}
	}

	_scene.add_camera(camera);
	
	camera->get_animation_tracks()->mType = TrackType::Camera;
	
	return camera;
}

std::shared_ptr<anim::Entity> SharedResources::create_light(const LightManager::DirectionalLight& parameters){

	auto root = get_root_entity();
	
	std::vector<std::string> existingNames; // Example existing actor names
	std::string prefix = "Light"; // Prefix for actor names
	
	auto children = root->get_mutable_children();
	
	// Example iteration to get existing names (replace this with your actual iteration logic)
	for (int i = 0; i < children.size(); ++i) {
		std::string actorName = children[i]->get_name();
		existingNames.push_back(actorName);
	}
	
	std::string uniqueActorName = anim::GenerateUniqueActorName(existingNames, prefix);
	
	
	auto modelPath = std::filesystem::path(DEFAULT_CWD) / "Models/Light.ubx";
	
	auto model = import_model(modelPath.string().c_str());
	auto entity = parse_model(model, modelPath.string().c_str(), false);
		
	auto light = std::dynamic_pointer_cast<DirectionalLightComponent>(entity->add_component<DirectionalLightComponent>());
	
	entity->get_component<TransformComponent>()->set_rotation(parameters.direction);
	entity->get_component<TransformComponent>()->set_translation({0.0f, 100.0f, 0.0f});

	light->set_parameters(parameters);
	
	entity->set_name(uniqueActorName);
	
	auto ui_shader = get_mutable_shader("ui");
	
	if(root){
		root->add_children(entity);
	}

	auto meshes = entity->get_children_recursive();
	for(auto child : meshes){
		if(auto mesh = child->get_component<MeshComponent>(); mesh){
			mesh->isInterface = true;
			mesh->set_shader(ui_shader.get());
		}
	}
	
	LightManager::getInstance()->addDirectionalLight(entity);
	
	
	_scene.add_light(entity);
	
	entity->get_animation_tracks()->mType = TrackType::Light;
	
	return entity;
}

}

