#ifndef ANIM_RESOURCES_SHARED_RESOURCES_H
#define ANIM_RESOURCES_SHARED_RESOURCES_H

#include "../graphics/mesh.h"

#include "shading/light_manager.h"

#include <memory>
#include <map>
#include <string>
#include <unordered_map>

class Scene;

struct DirectoryNode
{
	std::string FullPath;
	std::string FileName;
	std::vector<DirectoryNode> Children;
	bool IsDirectory;
};

struct SerializedModel {
	int EntityId;
	std::string Path;
};


namespace ui {
struct UiContext;
}

namespace anim
{

class Animator;
class Shader;
class Animation;
class Entity;
class Model;
struct ModelNode;
class PostProcessing;

struct AnimationSet {
	std::shared_ptr<Model> model;
	std::vector<std::shared_ptr<Animation>> animations;
};

class SharedResources : public std::enable_shared_from_this<SharedResources>
{
public:
	SharedResources(Scene& scene, const std::string& defaultScenePath = std::string{DEFAULT_CWD} + "/Scenes/Main.pwr");
	~SharedResources();
	
	void initialize();
	void refresh_directory_node();
	
	Animator *get_mutable_animator();
	std::shared_ptr<Shader> get_mutable_shader(const std::string &name);
	std::shared_ptr<Model> import_model(const char *path, float scale = 100.0f);
//	void import_animation(const char *path, float scale = 100.0f);
	void export_animation(std::shared_ptr<Entity> entity, const char *path, bool is_linear);
	void serialize(const std::string& path);
	void deserialize(const std::string& path);	
	std::shared_ptr<Entity> parse_model(std::shared_ptr<Model> &model, const char *path, bool serialize = true);
	void add_animations(const std::vector<std::shared_ptr<Animation>> &animations);
	void add_animation_set(const std::string& path, std::shared_ptr<Model> model, const std::vector<std::shared_ptr<Animation>> &animations);
	void add_animation(std::shared_ptr<Animation> animation);
	void add_shader(const std::string &name, const char *vs_path, const char *fs_path);
	
	AnimationSet& getAnimationSet(const std::string& path);
	const std::map<std::string, AnimationSet>& getAnimationSets();
	void convert_to_entity(std::shared_ptr<Entity> &entity,
						   std::shared_ptr<Model> &model,
						   const std::shared_ptr<ModelNode> &model_node,
						   std::shared_ptr<Entity> parent_entity, int child_num, std::shared_ptr<Entity> root_entity);
	void update(ui::UiContext& ui_context, bool includeSequencer = true);
	void set_ubo_position(const glm::vec3 &position);
	void set_ubo_projection(const glm::mat4 &projection);
	void set_ubo_view(const glm::mat4 &view);
	void set_dt(float dt);
	std::shared_ptr<Entity> get_root_entity();
	void set_root_entity(std::shared_ptr<Entity> entity);
	std::vector<std::shared_ptr<Animation>> &get_animations();
	std::shared_ptr<Animation> get_animation(int id);
	
	std::shared_ptr<Entity> get_entity(const std::string& name);
	std::shared_ptr<Entity> get_entity(int id);
	void remove_sub_entity(int id);
	void remove_entity(int id);

	std::unique_ptr<PostProcessing> mPostProcessing;
	
	const DirectoryNode& getDirectoryNode() {
		return rootNode;
	}
	
	void start_export_sequence(const std::string& filename);
	void offload_screen(int width, int height, unsigned int textureId);
	void end_export_sequence(bool isCancel);
	bool get_is_exporting(){
		return _exporting_sequence;
	}
	
	
	Scene& get_scene() const {
		return _scene;
	}
	
	std::shared_ptr<anim::Entity> create_camera(const glm::vec3& position = glm::vec3(0.0f, 5.0f, 250.0f));

	std::shared_ptr<anim::Entity> create_light(const LightManager::DirectionalLight& parameters = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0, 0.0f), glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(0.1f) });
		
private:
	void init_animator();
	void init_shader();
	std::unique_ptr<Animator> animator_;
	std::vector<std::shared_ptr<Animation>> animations_;
	std::map<std::string, std::shared_ptr<Shader>> shaders_;
	std::vector<std::shared_ptr<Entity>> single_entity_list_;
	std::shared_ptr<Entity> root_entity_;
	std::unique_ptr<Mesh> bone_;
	float dt_ = 0.0f;
	unsigned int matrices_UBO_;
	
	std::map<std::string, AnimationSet> animationSets_;
	
	DirectoryNode rootNode;
	
	bool _frame_exported = false;
	bool _exporting_sequence = false;
	std::string _export_path;
	
	int64_t _export_frame = 0;
	
	Scene& _scene;
	std::string _defaultScenePath;
	
	std::map<std::string, std::shared_ptr<Model>> _model_cache;
};

}
#endif
