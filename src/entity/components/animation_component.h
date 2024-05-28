#ifndef ANIM_ENTITY_COMPONENT_ANIMATION_COMPONENT_H
#define ANIM_ENTITY_COMPONENT_ANIMATION_COMPONENT_H

#include "component.h"
#include "entity/entity.h"
#include <memory>
#include <vector>
#include <map>
#include <fstream>

#include "shared_resources.h"
#include "scene/scene.hpp"
#include "json/json.h"

#include "glcpp/camera.h"
#include "components/renderable/light_component.h"

class Scene;

namespace anim
{
class Animation;
class AnimationComponent;
class Entity;
class SharedResources;

struct SequenceEntity {
	int mEntityId;
};

class ICinematicSequence {
public:
	virtual ~ICinematicSequence() = default;
	virtual ImSequencer::SequenceInterface& get_sequencer() = 0;
};

class ISerializableSequence : public ICinematicSequence {
public:
	virtual ~ISerializableSequence() = default;
	virtual void serialize() = 0;
	virtual void deserialize() = 0;
};

class CinematicSequence : public ISerializableSequence {
public:
	CinematicSequence(Scene& scene, std::shared_ptr<SharedResources> resources, const std::string& path) : scene_(scene), sequencer_(resources), _path(path), _resources(resources) {
		deserialize();
	}
	
	virtual ~CinematicSequence() override = default;
	
	// @TODO serialize deserialize when get/set
	TracksSequencer& get_sequencer() override {
		return sequencer_;
	}
	
	void serialize() override {
		Json::Value root;
		Json::Value entities(Json::arrayValue);
		Json::Value cameras(Json::arrayValue);
		Json::Value lights(Json::arrayValue);
		for (const auto& entity : mEntities) {
			Json::Value jsonEntity;
			jsonEntity["mEntityId"] = entity.mEntityId;
			jsonEntity["mType"] = (int)TrackType::Actor;

			auto entityPtr = _resources->get_entity(entity.mEntityId);

			Json::Value timeline(Json::arrayValue);

			if(entityPtr){
				jsonEntity["mTimeline"] = entityPtr->get_animation_tracks()->serialize();
			}
			
			entities.append(jsonEntity);
		}
		
		for (const auto& entity : mCameras) {
			Json::Value jsonEntity;
			jsonEntity["mEntityId"] = entity.mEntityId;
			jsonEntity["mType"] = (int)TrackType::Camera;

			auto entityPtr = scene_.get_camera(entity.mEntityId);

			if(entityPtr){
				jsonEntity["mTimeline"] = entityPtr->get_animation_tracks()->serialize();
			}
			
			cameras.append(jsonEntity);
		}

		for (const auto& entity : mLights) {
			Json::Value jsonEntity;
			jsonEntity["mEntityId"] = entity.mEntityId;
			jsonEntity["mType"] = (int)TrackType::Light;
			
			auto entityPtr = scene_.get_light(entity.mEntityId);
			
			if(entityPtr){
				jsonEntity["mTimeline"] = entityPtr->get_animation_tracks()->serialize();
			}
			
			lights.append(jsonEntity);
		}

		root["Sequencer"] = sequencer_.serialize();
		
		// Writing JSON data to the file specified by _path
		std::ofstream file(_path);
		if (!file.is_open()) {
			std::cerr << "Failed to open file for serialization: " << _path << std::endl;
			return;
		}
		Json::StreamWriterBuilder builder;
		builder["commentStyle"] = "None";
		builder["indentation"] = "    "; // Makes the output more human-readable
		std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
		writer->write(root, &file);
	}
	
	void deserialize() override {
		// Reading JSON data from the file specified by _path
		std::ifstream file(_path);
		if (!file.is_open()) {
			std::cerr << "Failed to open file for deserialization: " << _path << std::endl;
			return;
		}
		Json::Value root;
		file >> root;
		
		if (root.isNull() || root.empty()) {
			std::cerr << "Warning: File is empty or content is not valid JSON: " << _path << std::endl;
			// Optionally set your object to a default state here
			return;
		}
		
		// Deserialize TracksSequencer as well
		if (root.isMember("Sequencer")) {
			sequencer_.deserialize(root["Sequencer"]);
		}
	}

private:
	Scene& scene_;
	TracksSequencer sequencer_;

	std::vector<SequenceEntity> mEntities;
	std::vector<SequenceEntity> mCameras;
	std::vector<SequenceEntity> mLights;
	std::string _path;
	
	std::shared_ptr<SharedResources> _resources;
};

class AnimationSequence : public ISerializableSequence {
public:
	AnimationSequence(Scene& scene, std::shared_ptr<SharedResources> resources, std::shared_ptr<Entity> entity, const std::string& path, int animationId) : scene_(scene), sequencer_(resources), _entity(entity), _path(path), _resources(resources) {
		deserialize();
	}
	
	virtual ~AnimationSequence() override = default;
	
	// @TODO serialize deserialize when get/set
	ImSequencer::SequenceInterface& get_sequencer() override {
		return sequencer_;
	}
	
	void serialize() override;
	
	void deserialize() override;
	
	std::shared_ptr<Entity> get_root(){
		return _entity;
	}
	
	int get_animation_id(){
		return _animation_id;
	}
	
private:
	Scene& scene_;
	AnimationSequencer sequencer_;
	
	std::vector<SequenceEntity> mMeshes;
	std::vector<SequenceEntity> mBones;
	std::string _path;
	std::shared_ptr<SharedResources> _resources;
	
	std::shared_ptr<Entity> _entity;
	
	int _animation_id;
};

class RuntimeCinematicSequence {
public:
	RuntimeCinematicSequence(Scene& scene, std::shared_ptr<Entity> entity, std::shared_ptr<SharedResources> resources) : scene_(scene), sequencer_(resources), _resources(resources) {
		assert(entity->get_component<AnimationComponent>());
		mEntity = {entity->get_id()};
	}
	
	SequenceEntity& get_sequence() {
		return mEntity;
	}
	// @TODO serialize deserialize when get/set
	TracksSequencer& get_sequencer(){
		return sequencer_;
	}
	
private:
	Scene& scene_;
	TracksSequencer sequencer_;
	
	SequenceEntity mEntity;
	
	std::shared_ptr<SharedResources> _resources;
};

class StackedAnimation {
public:
	StackedAnimation(std::shared_ptr<Animation> animation, int startTime, int endTime, int offset);
	
	std::shared_ptr<Animation> get_wrapped_animation(){
		return _animation;
	}
	
	int get_start_time(){
		return _startTime;
	}
	
	int get_end_time() {
		return _endTime;
	}
	
	void set_weight(float weight){
		_weight = weight;
	}
	
	float get_weight() const {
		return _weight;
	}
	
	int get_duration() {
		return _duration;
	}

	int get_offset() {
		return _offset;
	}

private:
	std::shared_ptr<Animation> _animation;
	
	int _startTime;
	int _offset;
	int _endTime;
	int _duration;
	
	float _weight;
};

class AnimationComponent : public ComponentBase<AnimationComponent>
{
public:
	AnimationComponent();
	
	void init_animation();
	void play();
	void stop();
	void reload();
	void set_animation(std::shared_ptr<Animation> animation);
	void set_root_bone_name(const std::string& root_bone_name);
	void set_current_frame_num_to_time(uint32_t frame);
	void set_fps(float fps);
	std::shared_ptr<Animation> get_animation() const;
	bool get_is_loop();
	bool get_is_stop();
	const uint32_t get_current_frame_num() const;
	const uint32_t get_custom_duration() const;
	void set_current_time(float current_time);
	float get_current_time();
	float get_fps() const;
	float get_tps() const;

	const std::string& get_root_bone_name() const; 

	std::vector<std::vector<std::shared_ptr<StackedAnimation>>>& get_animation_stack_tracks() {
		return _animationStack;
	}
	
	void stack_track(std::vector<std::shared_ptr<StackedAnimation>>& stackedAnimation){
		_animationStack.push_back(stackedAnimation);
	}
	
	void clear_animation_stack(){
		_animationStack.clear();
	}
	

	void update_matrices(const std::vector<glm::mat4>& matrices){
		final_bone_matrices_ = matrices;
	}

	const std::vector<glm::mat4>& get_matrices() const {
		return final_bone_matrices_;
	}
	
	std::shared_ptr<Component> clone() override {
		auto clone = std::make_shared<AnimationComponent>();
		clone->fps_ = fps_;
		
		return clone;
	}
	
	
private:
	std::shared_ptr<Animation> animation_{nullptr};
	float current_time_ = 0.0f;
	float fps_ = 24.0f;
	bool is_stop_ = false;
	bool is_loop_ = true;
	
	std::vector<std::vector<std::shared_ptr<StackedAnimation>>> _animationStack;
	std::vector<glm::mat4> final_bone_matrices_;

};
}

#endif

