#define IMGUI_DEFINE_MATH_OPERATORS
#include "blueprint.h"
#include "scene/scene.hpp"

#include "animation_component.h"
#include "animation/animation.h"

#include "utility.h"


namespace BluePrint {

bool NodeProcessor::s_Running = false;

Node* NodeProcessor::SpawnGetActorNode(int actorId)
{
	
	auto entity = _scene.get_mutable_shared_resources()->get_entity(actorId);

	auto actorLabel = "Get " + entity->get_name();
	
	m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), actorLabel.c_str()));
	auto& node = *m_Nodes.back();
	node.Outputs.emplace_back(GetNextId(), node.ID, "Entity", PinType::Object, PinSubType::Actor);

	node.RootNode = true;
	
	int entityId = entity->get_id();
	
	node.Evaluate = [&node, this, entityId](){
		node.Outputs[0].Data = Entity{ entityId };
	};
	
	BuildNode(&node);
	
	return &node;
}

Node* NodeProcessor::SpawnGetAnimationNode(int animationId)
{
	auto entity = _scene.get_mutable_shared_resources()->get_animation(animationId);
	
	auto actorLabel = "Get " + entity->get_name();
	
	int entityId = entity->get_id();

	m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), actorLabel.c_str()));
	auto& node = *m_Nodes.back();
	node.Outputs.emplace_back(GetNextId(), node.ID, "Animation", PinType::Object, PinSubType::Animation);
	
	node.RootNode = true;
	
	node.Evaluate = [&node, this, entityId](){
		node.Outputs[0].Data = Entity{ entityId };
	};
	
	BuildNode(&node);
	
	return &node;
}

Node* NodeProcessor::SpawnGetActorPositionNode()
{
	m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), "Get Position"));
	auto& node = *m_Nodes.back();
	node.Inputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
	node.Inputs.emplace_back(GetNextId(), node.ID, "Actor", PinType::Object, PinSubType::Actor);
	node.Outputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
	node.Outputs.emplace_back(GetNextId(), node.ID, "X", PinType::Float);
	node.Outputs.emplace_back(GetNextId(), node.ID, "Y", PinType::Float);
	node.Outputs.emplace_back(GetNextId(), node.ID, "Z", PinType::Float);
		
	node.Evaluate = [&node, this](){
		node.Outputs[0].CanFlow = node.Inputs[0].CanFlow;
		node.Outputs[1].CanFlow = node.Outputs[0].CanFlow;
		node.Outputs[2].CanFlow = node.Outputs[0].CanFlow;
		node.Outputs[3].CanFlow = node.Outputs[0].CanFlow;
		
		if(node.Inputs[0].CanFlow){
			auto& data = node.Inputs[1].Data;
			if(data.has_value()){
				node.Inputs[1].CanFlow = true;
				node.Outputs[1].CanFlow = true;
				node.Outputs[2].CanFlow = true;
				node.Outputs[3].CanFlow = true;
				
				auto actor = std::get<Entity>(*data);
				
				auto entity = _scene.get_mutable_shared_resources()->get_entity(actor.Id);
				
				if(entity){
					auto [t, r, s] = anim::DecomposeTransform(entity->get_world_transformation());
					node.Outputs[1].Data = t.x;
					node.Outputs[2].Data = t.y;
					node.Outputs[3].Data = t.z;
				} else{
					node.Outputs[1].Data = std::nullopt;
					node.Outputs[2].Data = std::nullopt;
					node.Outputs[3].Data = std::nullopt;
					
					node.Outputs[1].CanFlow = false;
					node.Outputs[2].CanFlow = false;
					node.Outputs[3].CanFlow = false;
				}
			} else {
				node.Inputs[1].CanFlow = false;
				node.Outputs[1].CanFlow = false;
				node.Outputs[2].CanFlow = false;
				node.Outputs[3].CanFlow = false;
			}
		}
	};
	
	BuildNode(&node);
	
	return &node;
}


Node* NodeProcessor::SpawnSetActorPositionNode()
{
	m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), "Set Position"));
	auto& node = *m_Nodes.back();
	node.Inputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
	node.Inputs.emplace_back(GetNextId(), node.ID, "Actor", PinType::Object, PinSubType::Actor);
	node.Inputs.emplace_back(GetNextId(), node.ID, "X", PinType::Float);
	node.Inputs.emplace_back(GetNextId(), node.ID, "Y", PinType::Float);
	node.Inputs.emplace_back(GetNextId(), node.ID, "Z", PinType::Float);
	node.Outputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
		
	node.Evaluate = [&node, this](){
		node.Outputs[0].CanFlow = node.Inputs[0].CanFlow;

		if(node.Inputs[0].CanFlow){
			auto& data = node.Inputs[1].Data;
			if(data.has_value()){
				node.Inputs[1].CanFlow = true;
				node.Inputs[2].CanFlow = true;
				node.Inputs[3].CanFlow = true;
				node.Inputs[4].CanFlow = true;

				auto actor = std::get<Entity>(*data);

				auto entity = _scene.get_mutable_shared_resources()->get_entity(actor.Id);
				
				if(entity){
					auto [t, r, s] = anim::DecomposeTransform(entity->get_world_transformation());
					
					float x = t.x;
					float y = t.y;
					float z = t.z;
					
					if(node.Inputs[2].Data.has_value()){
						x = std::get<float>(*node.Inputs[2].Data);
					}

					if(node.Inputs[3].Data.has_value()){
						y = std::get<float>(*node.Inputs[3].Data);
					}

					
					if(node.Inputs[4].Data.has_value()){
						z = std::get<float>(*node.Inputs[4].Data);
					}
					
					t.x = x;
					t.y = y;
					t.z = z;
					
					auto world = anim::ComposeTransform(t, r, s);
					
					entity->set_local(world);
					
				}
			} else {
				node.Inputs[1].CanFlow = false;
				node.Inputs[2].CanFlow = false;
				node.Inputs[3].CanFlow = false;
				node.Inputs[4].CanFlow = false;
			}
		}
	};
	
	BuildNode(&node);
	
	return &node;
}


Node* NodeProcessor::SpawnGetActorRotationNode()
{
	m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), "Get Position"));
	auto& node = *m_Nodes.back();
	node.Inputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
	node.Inputs.emplace_back(GetNextId(), node.ID, "Actor", PinType::Object, PinSubType::Actor);
	node.Outputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
	node.Outputs.emplace_back(GetNextId(), node.ID, "X", PinType::Float);
	node.Outputs.emplace_back(GetNextId(), node.ID, "Y", PinType::Float);
	node.Outputs.emplace_back(GetNextId(), node.ID, "Z", PinType::Float);
	
	node.Evaluate = [&node, this](){
		node.Outputs[0].CanFlow = node.Inputs[0].CanFlow;
		node.Outputs[1].CanFlow = node.Outputs[0].CanFlow;
		node.Outputs[2].CanFlow = node.Outputs[0].CanFlow;
		node.Outputs[3].CanFlow = node.Outputs[0].CanFlow;
		
		if(node.Inputs[0].CanFlow){
			auto& data = node.Inputs[1].Data;
			if(data.has_value()){
				node.Inputs[1].CanFlow = true;
				node.Outputs[1].CanFlow = true;
				node.Outputs[2].CanFlow = true;
				node.Outputs[3].CanFlow = true;
				
				auto actor = std::get<Entity>(*data);
				
				auto entity = _scene.get_mutable_shared_resources()->get_entity(actor.Id);
				
				if(entity){
					auto [t, r, s] = anim::DecomposeTransform(entity->get_local());
					
					auto euler = glm::degrees(glm::eulerAngles(r));
					
					node.Outputs[1].Data = euler.x;
					node.Outputs[2].Data = euler.y;
					node.Outputs[3].Data = euler.z;
				} else{
					node.Outputs[1].Data = std::nullopt;
					node.Outputs[2].Data = std::nullopt;
					node.Outputs[3].Data = std::nullopt;
					
					node.Outputs[1].CanFlow = false;
					node.Outputs[2].CanFlow = false;
					node.Outputs[3].CanFlow = false;
				}
			} else {
				node.Inputs[1].CanFlow = false;
				node.Outputs[1].CanFlow = false;
				node.Outputs[2].CanFlow = false;
				node.Outputs[3].CanFlow = false;
			}
		}
	};
	
	BuildNode(&node);
	
	return &node;
}

Node* NodeProcessor::SpawnSetActorRotationNode()
{
	m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), "Set Position"));
	auto& node = *m_Nodes.back();
	node.Inputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
	node.Inputs.emplace_back(GetNextId(), node.ID, "Actor", PinType::Object, PinSubType::Actor);
	node.Inputs.emplace_back(GetNextId(), node.ID, "X", PinType::Float);
	node.Inputs.emplace_back(GetNextId(), node.ID, "Y", PinType::Float);
	node.Inputs.emplace_back(GetNextId(), node.ID, "Z", PinType::Float);
	node.Outputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
	
	node.Evaluate = [&node, this](){
		node.Outputs[0].CanFlow = node.Inputs[0].CanFlow;
		
		if(node.Inputs[0].CanFlow){
			auto& data = node.Inputs[1].Data;
			if(data.has_value()){
				node.Inputs[1].CanFlow = true;
				node.Inputs[2].CanFlow = true;
				node.Inputs[3].CanFlow = true;
				node.Inputs[4].CanFlow = true;
				
				auto actor = std::get<Entity>(*data);
				
				auto entity = _scene.get_mutable_shared_resources()->get_entity(actor.Id);
				
				if(entity){
					
					auto rotation = entity->get_component<anim::TransformComponent>()->mRotation;

					auto euler = glm::degrees(rotation);
					
					float x = euler.x;
					float y = euler.y;
					float z = euler.z;
					
					if(node.Inputs[2].Data.has_value()){
						x = std::get<float>(*node.Inputs[2].Data);
					}
					
					if(node.Inputs[3].Data.has_value()){
						y = std::get<float>(*node.Inputs[3].Data);
					}
					
					
					if(node.Inputs[4].Data.has_value()){
						z = std::get<float>(*node.Inputs[4].Data);
					}
					
					euler = glm::vec3(x, y, z);
					
					auto radians = glm::radians(euler);
					
					entity->get_component<anim::TransformComponent>()->mRotation = radians;
					
				}
			} else {
				node.Inputs[1].CanFlow = false;
				node.Inputs[2].CanFlow = false;
				node.Inputs[3].CanFlow = false;
				node.Inputs[4].CanFlow = false;
			}
		}
	};
	
	BuildNode(&node);
	
	return &node;
}

Node* NodeProcessor::SpawnPlayAnimationNode(){
	m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), "Play Animation"));
	auto& node = *m_Nodes.back();
	node.Inputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
	node.Inputs.emplace_back(GetNextId(), node.ID, "Entity", PinType::Object, PinSubType::Actor);
	node.Inputs.emplace_back(GetNextId(), node.ID, "Animation", PinType::Object, PinSubType::Animation);
	node.Outputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
	
	node.Evaluate = [&node, this](){
		node.Outputs[0].CanFlow = node.Inputs[0].CanFlow;
		
		if(node.Inputs[0].CanFlow){
			auto& entityData = node.Inputs[1].Data;
			auto& animationData = node.Inputs[2].Data;
			if(animationData.has_value() && entityData.has_value()){				
				auto inputSequence = std::get<Entity>(*animationData);
				auto inputEntity = std::get<Entity>(*entityData);
				
				auto entity = _scene.get_mutable_shared_resources()->get_entity(inputEntity.Id);
				
				if(entity){
					auto& scene = _scene;
					
					int animationId = inputSequence.Id;

					auto resources = scene.get_mutable_shared_resources();
					auto sequence = std::make_shared<anim::RuntimeCinematicSequence>(_scene, entity, resources);

					auto& sequencer = sequence->get_sequencer();

					auto animation = resources->get_animation(animationId);

					sequencer.mItems.push_back(std::make_shared<TracksContainer>(entity->get_id(), entity->get_name(), TrackType::Actor, ImSequencer::SEQUENCER_OPTIONS::SEQUENCER_EDIT_NONE));
										
					auto subSequencer = sequencer.mItems.back();
					
					subSequencer->mTracks.push_back(std::make_shared<SequenceItem>(animation->get_id(), animation->get_name(), ItemType::Animation, 0, (int)animation->get_duration()));
					
					sequencer.mFrameMax = (int)animation->get_duration();
					
					m_SequenceStack.push_back(sequence);
				}
			}
		}
		
		node.Inputs[1].CanFlow = node.Inputs[0].CanFlow;
		node.Inputs[2].CanFlow = node.Inputs[0].CanFlow;
	};
	
	BuildNode(&node);
	
	return &node;
}

Node* NodeProcessor::SpawnAddNode(){
	m_Nodes.emplace_back(std::make_unique<Node>(GetNextId(), "Add"));
	auto& node = *m_Nodes.back();
	node.Inputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
	node.Inputs.emplace_back(GetNextId(), node.ID, "A", PinType::Float);
	node.Inputs.emplace_back(GetNextId(), node.ID, "B", PinType::Float);
	node.Outputs.emplace_back(GetNextId(), node.ID, "", PinType::Flow);
	node.Outputs.emplace_back(GetNextId(), node.ID, "=", PinType::Float);
	
	node.Evaluate = [&node, this](){
		node.Outputs[0].CanFlow = node.Inputs[0].CanFlow;
		node.Outputs[1].CanFlow = node.Outputs[0].CanFlow;

		if(node.Inputs[0].CanFlow){
			node.Inputs[1].CanFlow = true;
			node.Inputs[2].CanFlow = true;
			node.Outputs[1].CanFlow = true;
			if(node.Inputs[1].Data.has_value() && node.Inputs[2].Data.has_value()){
				float a = std::get<float>(*node.Inputs[1].Data);
				float b = std::get<float>(*node.Inputs[2].Data);
				node.Outputs[1].Data = a + b;
			} else if(node.Inputs[1].Data.has_value()){
				float a = std::get<float>(*node.Inputs[1].Data);
				node.Outputs[1].Data = a;
				node.Inputs[2].CanFlow = false;
			} else if(node.Inputs[2].Data.has_value()){
				float b = std::get<float>(*node.Inputs[2].Data);
				node.Outputs[1].Data = b;
				node.Inputs[1].CanFlow = false;
			} else {
				node.Outputs[1].Data = std::nullopt;
				node.Outputs[1].CanFlow = false;
				node.Inputs[1].CanFlow = false;
				node.Inputs[2].CanFlow = false;
			}
		}
	};
	
	BuildNode(&node);
	
	return &node;
	
}

}
