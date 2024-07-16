#include "entity.h"
#include "animator.h"
#include "animation/animation.h"
#include "components/animation_component.h"
#include "components/renderable/physics_component.h"

#include "utility.h"

#include <glm/gtc/type_ptr.hpp>

namespace anim
{

TracksSequencer::TracksSequencer(std::shared_ptr<SharedResources> resources) :
mFrameMax(300), _resources(resources){
	
	OnMissingActorPayload = [this](int missingIndex, const ImGuiPayload* payload, int type){
			
			if(mCompositionStage == CompositionStage::SubSequence){
				return;
			}
			
			const char* id_data = reinterpret_cast<const char*>(payload->Data);
			
			int actor_id = std::atoi(id_data);
			
			auto entity = _resources->get_entity(actor_id);
			
			if(entity){
				
				if(missingIndex != -1){
					
					if(mItems[missingIndex]->mType != static_cast<TrackType>(type)){
						return;
					}
					
					auto sequence = _resources->get_mutable_animator()->get_active_cinematic_sequence();
					
					auto& sequencer = static_cast<TracksSequencer&>(sequence->get_sequencer());
					
					for(auto& track : sequencer.mItems){
						if(track->mId == mItems[missingIndex]->mId){
							track->mId = entity->get_id();
						}
					}
					
					mItems[missingIndex]->mId = entity->get_id();
					mItems[missingIndex]->mName = entity->get_name();
					mItems[missingIndex]->mMissing = false;
					
					auto subSequencer = entity->get_animation_tracks();
					subSequencer->mTracks.clear();
					subSequencer->mTracks = mItems[missingIndex]->mTracks;
					
				}
			}
		};
	
}

Json::Value TracksSequencer::serialize() const  {
	Json::Value root;
	root["mFrameMax"] = mFrameMax;
	Json::Value items(Json::arrayValue);
	for (const auto& item : mItems) {
		Json::Value jsonItem = item->serialize();
		items.append(jsonItem);
	}
	
	root["mItems"] = items;
	
	return root;
}

void TracksSequencer::deserialize(const Json::Value& root)
{
	mFrameMax = root["mFrameMax"].asInt();
	mItems.clear();
	const Json::Value items = root["mItems"];
	for (const auto& jsonItem : items) {
		std::shared_ptr<TracksContainer> item = std::make_shared<TracksContainer>();
		
		item->deserialize(jsonItem);
		
		if(item->mType == TrackType::Actor){
			auto entity = _resources->get_entity(item->mId);
			if(!entity){
				item->mMissing = true;
			}
			
			if(entity){
				entity->get_animation_tracks() = item;
			}
		}
		
		if(item->mType == TrackType::Camera){
			auto& scene = _resources->get_scene();
			auto entity = scene.get_camera(item->mId);
			
			if(!entity){
				item->mMissing = true;
			}
			
			if(entity){
				entity->get_animation_tracks() = item;
			}
		}
		
		if(item->mType == TrackType::Light){
			auto& scene = _resources->get_scene();
			auto entity = scene.get_light(item->mId);
			
			if(!entity){
				item->mMissing = true;
			}
			
			if(entity){
				entity->get_animation_tracks() = item;
			}
		}
		
		item->mOptions = static_cast<ImSequencer::SEQUENCER_OPTIONS>(jsonItem["mOptions"].asInt());
		
		mItems.push_back(item);
	}
}
AnimationSequencer::AnimationSequencer(std::shared_ptr<SharedResources> resources) :
mFrameMax(300), _resources(resources){
	
}

Entity::Entity(std::shared_ptr<SharedResources> resources, const std::string& name, int identifier, std::shared_ptr<Entity> parent)
: name_(name), id_(identifier), parent_(parent), scene_(resources->get_scene()){
	_transform_component = add_component<TransformComponent>();
	sequence_items_ = std::make_shared<TracksContainer>();
}
void Entity::update()
{
	if (is_deactivate_)
	{
		return;
	}
	world_ = get_local();
	if (parent_)
	{
		world_ = parent_->get_world_transformation() * world_;
	}
	for (auto& component : components_)
	{
		if(component->get_type() != PhysicsComponent::type){
			component->update(shared_from_this());
		}
	}
	for (auto& child : children_)
	{
		child->update();
	}
	
	for (auto& component : components_)
	{
		if(component->get_type() == PhysicsComponent::type){
			component->update(shared_from_this());
		}
	}
}
std::shared_ptr<Entity> Entity::find(const std::string& name)
{
	return find(root_, name);
}
std::shared_ptr<Entity> Entity::find(std::shared_ptr<Entity> entity, const std::string& name)
{
	if (!entity || entity->name_ == name)
	{
		return entity;
	}
	std::shared_ptr<Entity> ret = nullptr;
	for (auto& children : entity->children_)
	{
		ret = find(children, name);
		if (ret)
		{
			return ret;
		}
	}
	return ret;
}

void Entity::set_name(const std::string& name)
{
	name_ = name;
}
void Entity::add_children(std::shared_ptr<Entity>& child)
{
	if (child->id_ != this->id_)
	{
		child->parent_ = shared_from_this();
		
		auto it = std::find_if(children_.begin(), children_.end(),
							   [&child](const std::shared_ptr<Entity>& currentChild)
							   {
			return currentChild->get_id() == child->get_id();
		});
		
		if (it == children_.end())
		{
			children_.push_back(child);
		}
	}
	
	assert(child != child->get_mutable_parent());
}

void Entity::removeChild(const std::shared_ptr<Entity>& child)
{
	auto it = std::find_if(children_.begin(), children_.end(),
						   [&child](const std::shared_ptr<Entity>& currentChild)
						   {
		return currentChild->get_id() == child->get_id();
	});
	
	if (it != children_.end())
	{
		(*it)->parent_ = nullptr;
		children_.erase(it);
	}
}

}
