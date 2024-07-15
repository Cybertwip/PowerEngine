#include <cmath>

#include "PhysicsWorld.h"

#include "Geometry.h"
#include "GeometryExtensions.h"

#include "entity/entity.h"


#include "components/transform_component.h"
#include "components/renderable/physics_component.h"

using namespace windy;

using namespace physics;


std::shared_ptr<PhysicsWorld> PhysicsWorld::create() {
	
	auto physicsWorld = std::shared_ptr<PhysicsWorld>(new (std::nothrow) PhysicsWorld());
	
	if (physicsWorld && physicsWorld->init()) {
		physicsWorld->_contactEventCollisionIndex = 0;
		return physicsWorld;
	}
	
	return nullptr;
}

bool PhysicsWorld::init()
{
	this->_gravity = 8;
	this->_maxFallSpeed = 6;
	
	return true;
}

void PhysicsWorld::setUnderwater(bool underwater) {
	if (underwater) {
		this->_gravity = 4;
		this->_maxFallSpeed = 3;
	}
	else {
		this->_gravity = 8;
		this->_maxFallSpeed = 6;
	}
}

void PhysicsWorld::alignCollisions(std::shared_ptr<anim::Entity> a, std::shared_ptr<anim::Entity> b, bool clearContacts) {
	auto entity = a->get_component<anim::PhysicsComponent>();
	auto landscapeEntity = b->get_component<anim::PhysicsComponent>();
	auto transform = a->get_component<anim::TransformComponent>();
	
	for (int x = 0; x < entity->get_collision_size(); ++x) {
		auto collisionBox = entity->get_collision_box(x);
		
		for (int y = 0; y < landscapeEntity->get_collision_size(); ++y) {
			auto landscapeEntityCollisionBox = landscapeEntity->get_collision_box(y);
			
			for (int k = 0; k < 8; k++) {
				windy::Rect collisionBoxTiles[9];
				
				for (int j = 0; j < 9; ++j) {
					int column = j % 3;
					int row = static_cast<int>(std::floor(j / 3));
					
					float tileW = collisionBox->size.x / 3.0f;
					float tileH = collisionBox->size.y / 3.0f;
					
					float collisionBoxLeft = collisionBox->getMinX();
					float collisionBoxTop = collisionBox->getMaxY();
					
					auto tileOrigin = glm::vec2(collisionBoxLeft + (tileW * column), collisionBoxTop - (tileH * row));
					
					collisionBoxTiles[j] = windy::Rect(tileOrigin.x, tileOrigin.y - tileH, tileW, tileH);
				}
				
				windy::Rect slicedCollisionBox[8];
				
				slicedCollisionBox[0] = collisionBoxTiles[7];
				slicedCollisionBox[1] = collisionBoxTiles[1];
				slicedCollisionBox[2] = collisionBoxTiles[3];
				slicedCollisionBox[3] = collisionBoxTiles[5];
				slicedCollisionBox[4] = collisionBoxTiles[0];
				slicedCollisionBox[5] = collisionBoxTiles[2];
				slicedCollisionBox[6] = collisionBoxTiles[6];
				slicedCollisionBox[7] = collisionBoxTiles[8];
				
				if (windy::GeometryExtensions::rectIntersectsRect(*landscapeEntityCollisionBox, slicedCollisionBox[k])) {
					auto intersection = GeometryExtensions::rectIntersection(*landscapeEntityCollisionBox, slicedCollisionBox[k]);
					
					bool hasOffsetX = false;
					bool hasOffsetY = false;
					
					windy::Contact contactType;
					
					switch (k) {
						case 0:
							contactType = windy::Contact::Up;
							hasOffsetY = true;
							break;
						case 1:
							contactType = windy::Contact::Down;
							hasOffsetY = true;
							intersection.size.y *= -1;
							break;
						case 2:
							contactType = windy::Contact::Left;
							hasOffsetX = true;
							break;
						case 3:
							contactType = windy::Contact::Right;
							hasOffsetX = true;
							intersection.size.x *= -1;
							break;
						default:
							if (intersection.size.x >= intersection.size.y) {
								if (k == 4 || k == 5) {
									contactType = windy::Contact::Down;
									hasOffsetY = true;
									intersection.size.y = -intersection.size.y;
								} else {
									contactType = windy::Contact::Up;
									hasOffsetY = true;
								}
							} else {
								if (k == 7 || k == 5) {
									contactType = windy::Contact::Right;
									hasOffsetX = true;
									intersection.size.x *= -1;
								} else {
									contactType = windy::Contact::Left;
									hasOffsetX = true;
								}
								
							}
					}
					
					auto contact = std::make_shared<windy::CollisionContact>();
					contact->type = contactType;
					contact->contact = true;
					contact->entity = b;
					entity->get_contacts_internal().push_back(contact);
					
					if (hasOffsetX) {
						auto transformTranslation = transform->get_translation();
						
						transformTranslation.x += intersection.size.x;
						
						transform->set_translation(transformTranslation);
						collisionBox->origin.x += intersection.size.x;
					}
					
					if (hasOffsetY) {
						
						auto transformTranslation = transform->get_translation();
						
						transformTranslation.y += intersection.size.y;

						transform->set_translation(transformTranslation);

						collisionBox->origin.y += intersection.size.y;
					}
				}
			}
		}
	}
}

void PhysicsWorld::update(float dt)
{
	std::vector<std::shared_ptr<anim::Entity>> landscapeEntities;
	std::vector<std::shared_ptr<anim::Entity>> specialLandscapeEntities;
	std::vector<std::shared_ptr<anim::Entity>> collidingEntities;
	
	for (int i = 0; i < _entities.size(); ++i) {
		auto entity = _entities.at(i);
		
		auto physics = entity->get_component<anim::PhysicsComponent>();
		
		if(physics->is_dynamic()){
			collidingEntities.push_back(entity);
		} else {
			landscapeEntities.push_back(entity);
		}
	}
	
	for (int i = 0; i < collidingEntities.size(); ++i) {
		auto entity = collidingEntities.at(i);
		auto physics = entity->get_component<anim::PhysicsComponent>();
		
		physics->recreate_contacts();
		
		for (int j = 0; j < landscapeEntities.size(); ++j) {
			auto landscapeEntity = landscapeEntities.at(j);
			
			if(entity != landscapeEntity){
				PhysicsWorld::alignCollisions(entity, landscapeEntity);
			}
		}
	}
	
	// Apply speed and gravity
	for (int i = 0; i < collidingEntities.size(); ++i) {
		auto entity = collidingEntities.at(i);
		auto physics = entity->get_component<anim::PhysicsComponent>();
		if(physics->is_dynamic()){
			auto transform = entity->get_component<anim::TransformComponent>();
			
			auto entityPosition = transform->get_translation();
			
			physics->get_speed().y  -= this->_gravity * dt;
			
			if (physics->get_speed().y >= this->_maxFallSpeed) {
				physics->get_speed().y = this->_maxFallSpeed;
			}
			
			// Cap speed
			if (physics->get_contact(windy::Contact::Down)  || physics->get_contact(windy::Contact::Up) ) {
				physics->get_speed().y = 0;
			}
			
			if (physics->get_contact(windy::Contact::Left)  || physics->get_contact(windy::Contact::Right) ) {
				physics->get_speed().x = 0;
			}
			
			
			entityPosition += physics->get_speed() * dt;
			
			transform->set_translation(entityPosition);
		}
		
	}
	
	for (int i = 0; i < collidingEntities.size(); ++i) {
		auto entity = collidingEntities.at(i);
		
		auto entityPhysics = entity->get_component<anim::PhysicsComponent>();
		
		for (int j = 0; j < collidingEntities.size(); ++j) {
			auto collidingEntity = collidingEntities.at(j);
			
			auto collidingPhysics = collidingEntity->get_component<anim::PhysicsComponent>();
			
			if (entity == collidingEntity) {
				continue;
			}
			
			for(int x = 0; x < entityPhysics->get_collision_size(); ++x){
				
				auto entityCollisionBox = entityPhysics->get_collision_box(x);
				auto collidingCollisionBox = collidingPhysics->get_collision_box(x);
				
				if (GeometryExtensions::rectIntersectsRect(*entityCollisionBox, *collidingCollisionBox)) {
					
					
					auto iterator =
					std::find_if(this->_contactEventCollisions.begin(),
								 this->_contactEventCollisions.end(), [=](const std::pair<long long, std::pair<std::shared_ptr<anim::Entity>, std::shared_ptr<anim::Entity>>> contact) {
						return contact.second.first == entity && contact.second.second == collidingEntity;
					});
					
					if (iterator == _contactEventCollisions.end()) {
						
						long long contactIndex = 0;
						long long emptyIndex = 0;
						
						bool indexFound = false;
						
						for (long long k = 0; k <= this->_contactEventCollisionIndex; ++k) {
							
							indexFound = false;
							
							for (unsigned int l = 0; l < this->_contactEventCollisions.size(); ++l) {
								auto contact = this->_contactEventCollisions.at(l);
								
								if (contact.first == k) {
									indexFound = true;
									break;
								}
							}
							
							if (!indexFound) {
								emptyIndex = k;
								break;
							}
							
						}
						
						if (!indexFound) {
							contactIndex = emptyIndex;
						}
						else {
							contactIndex = this->_contactEventCollisionIndex + 1;
							this->_contactEventCollisionIndex += 1;
						}
						
						this->_contactEventCollisions.push_back({ contactIndex, { entity, collidingEntity } });
						//                    entity->onCollisionEnter(collidingEntity);
						
					}
				}
			}
			
		}
	}
	
	
	this->_contactEventCollisions.erase(
										std::remove_if(this->_contactEventCollisions.begin(),
													   this->_contactEventCollisions.end(),
													   [=](const std::pair<long long, std::pair<std::shared_ptr<anim::Entity>, std::shared_ptr<anim::Entity>>> contact) {
														   
														   bool entityExists = std::find(_entities.begin(), _entities.end(),contact.second.first) != _entities.end();
														   bool collisionExists = std::find(_entities.begin(), _entities.end(), contact.second.second) != _entities.end();
														   
														   return !entityExists || !collisionExists;
													   }),
										this->_contactEventCollisions.end());
	
	std::vector<std::pair<long long, std::pair<std::shared_ptr<anim::Entity>, std::shared_ptr<anim::Entity>>>> contactExitCollisions;
	
	for (unsigned int i = 0; i < this->_contactEventCollisions.size(); ++i) {
		auto contact = this->_contactEventCollisions.at(i);
		
		auto entity = contact.second.first;
		auto collidingEntity = contact.second.second;
		
		auto entityPhysics = entity->get_component<anim::PhysicsComponent>();
		auto collidingPhysics = collidingEntity->get_component<anim::PhysicsComponent>();
		
		for(int x = 0; x < entityPhysics->get_collision_size(); ++x){
			if (GeometryExtensions::rectIntersectsRect(*entityPhysics->get_collision_box(x), *collidingPhysics->get_collision_box(x))) {
				
				//            entity->onCollision(collidingEntity);
			}
			else {
				contactExitCollisions.push_back(contact);
			}
		}
		
	}
	
	this->_contactEventCollisions.erase(
										std::remove_if(this->_contactEventCollisions.begin(),
													   this->_contactEventCollisions.end(),
													   [=](const std::pair<long long, std::pair<std::shared_ptr<anim::Entity>, std::shared_ptr<anim::Entity>>> contact) {
														   
														   bool shouldRemove = false;
														   for (unsigned int i = 0; i < contactExitCollisions.size(); ++i) {
															   auto validationContact = contactExitCollisions.at(i);
															   
															   shouldRemove = validationContact.first == contact.first &&
															   validationContact.second.first == contact.second.first &&
															   validationContact.second.second == contact.second.second;
															   
															   if (shouldRemove) {
																   break;
															   }
															   
														   }
														   
														   return shouldRemove;
													   }),
										this->_contactEventCollisions.end());
	
	
	for (unsigned int i = 0; i < contactExitCollisions.size(); ++i) {
		auto contact = contactExitCollisions.at(i);
		
		auto entity = contact.second.first;
		auto collidingEntity = contact.second.second;
		
		//        entity->onCollisionExit(collidingEntity);
	}
	
}

void PhysicsWorld::addEntity(std::shared_ptr<anim::Entity> entity) {
	_entities.push_back(entity);
}

void PhysicsWorld::removeEntity(std::shared_ptr<anim::Entity> entity) {
	auto it = std::find(_entities.begin(), _entities.end(), entity);
	
	if(it != _entities.end()){
		_entities.erase(it);
	}
}


void PhysicsWorld::unregisterContact(std::shared_ptr<anim::Entity> a, std::shared_ptr<anim::Entity> b) {
	this->_contactEventCollisions.erase(
										std::remove_if(this->_contactEventCollisions.begin(),
													   this->_contactEventCollisions.end(),
													   [=](const std::pair<long long, std::pair<std::shared_ptr<anim::Entity>, std::shared_ptr<anim::Entity>>> contact) {
														   
														   bool shouldRemove = false;
														   shouldRemove = (contact.second.first == a &&
																		   contact.second.second == b) ||
														   (contact.second.first == b &&
															contact.second.second == a);
														   
														   return shouldRemove;
													   }),
										this->_contactEventCollisions.end());
	
}

void PhysicsWorld::unregisterContact(std::shared_ptr<anim::Entity> a) {
	this->_contactEventCollisions.erase(
										std::remove_if(this->_contactEventCollisions.begin(),
													   this->_contactEventCollisions.end(),
													   [=](const std::pair<long long, std::pair<std::shared_ptr<anim::Entity>, std::shared_ptr<anim::Entity>>> contact) {
														   
														   bool shouldRemove = false;
														   shouldRemove = (contact.second.first == a || contact.second.second == a);
														   
														   if (shouldRemove) {
															   //                    contact.second.first->onCollisionExit(contact.second.second);
														   }
														   
														   return shouldRemove;
													   }),
										this->_contactEventCollisions.end());
}


void PhysicsWorld::resetContactEventCollisions() {
	this->_contactEventCollisions.clear();
}

