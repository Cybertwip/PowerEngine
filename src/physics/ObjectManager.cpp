#include <algorithm>

#include "ObjectManager.h"

#include "Level.h"
#include "PhysicsWorld.h"

#include "Entities/Logical.h"
#include "Entities/Bounds.h"

#include "Entities/ObjectEntry.h"

#include "GameTags.h"

#include "GeometryExtensions.h"

using namespace windy;

std::shared_ptr<ObjectManager> ObjectManager::create(std::shared_ptr<Level> level) {

    auto objectManager = std::shared_ptr<ObjectManager>(new (std::nothrow) ObjectManager());

    if (objectManager) {
        objectManager->_level = level;
    }

    if (objectManager && objectManager->init()) {
        return objectManager;
    }

    return nullptr;
}

bool ObjectManager::init()
{
    //////////////////////////////
    // 1. super init first
    if (!Node::init())
    {
        return false;
    }

    return true;
}


void ObjectManager::resetEntryTable(std::vector<std::shared_ptr<ObjectEntry>> newEntries) {

    for (unsigned int i = 0; i < newEntries.size(); ++i) {
        newEntries[i]->setFinished(false);
        newEntries[i]->setFinishedForever(false);
        newEntries[i]->setRespawnPreventionFlag(false);
    }

    for (unsigned int i = 0; i < this->_objectEntries.size(); ++i) {
        auto entry = _objectEntries[i];

        _objectEntries[i]->setFinished(false);
        _objectEntries[i]->setRespawnPreventionFlag(false);

        if (entry->getMappedInstance() != nullptr) {

			auto& entities = _level->getPhysicsWorld()->getEntities();
			auto it = std::find(entities.begin(), entities.end(), entry->getMappedInstance());

			_level->getPhysicsWorld()->getEntities().erase(it);
            _level->getPhysicsWorld()->unregisterContact(entry->getMappedInstance());

            entry->getMappedInstance()->onFinished();
            entry->getMappedInstance()->removeFromParent();
            entry->setMappedInstance(nullptr);
        }
    }

    _objectEntries = newEntries;

}

void ObjectManager::addEntry(std::shared_ptr<ObjectEntry> entry) {
    _objectEntries.push_back(entry);
}


void ObjectManager::update(float dt)
{
    // Erase finished forever entities

    for (unsigned int i = 0; i < _objectEntries.size(); ++i) {
        auto entry = _objectEntries[i];

        if (entry->getIsFinishedForever()) {
            if (entry->getMappedInstance()!= nullptr) {
                entry->getMappedInstance()->onFinished();

				auto& entities = _level->getPhysicsWorld()->getEntities();
				auto it = std::find(entities.begin(), entities.end(), entry->getMappedInstance());

                _level->getPhysicsWorld()->getEntities().erase(it);
                _level->getPhysicsWorld()->unregisterContact(entry->getMappedInstance());

                entry->getMappedInstance()->removeFromParent();
                entry->setMappedInstance(nullptr);
            }
        }
    }

    // Erase finished forever entries
    _objectEntries.erase(std::remove_if(_objectEntries.begin(), 
                                             _objectEntries.end(), 
                                             [](const std::shared_ptr<ObjectEntry> entry) {
                                                 return entry->getIsFinishedForever();
                                             }), 
                              _objectEntries.end());



    // We have to inflate the bounds by 96 pixels for intro and outro player position (offscreen location)

    auto boundsCollisionBox = _level->getBounds()->getCollisionBox();
    auto inflatedBoundsCollisionBox = _level->getBounds()->inflate(windy::Size(16, 16));

    for (unsigned int i = 0; i < _objectEntries.size(); ++i) {
        auto entry = _objectEntries[i];

        if (entry->getIsFinished() && entry->getMappedInstance() != nullptr) {
            entry->getMappedInstance()->onFinished();

			auto& entities = _level->getPhysicsWorld()->getEntities();

			auto it = std::find(entities.begin(), entities.end(), entry->getMappedInstance());
			_level->getPhysicsWorld()->getEntities().erase(it);
            _level->getPhysicsWorld()->unregisterContact(entry->getMappedInstance());

            entry->getMappedInstance()->removeFromParent();
            entry->setMappedInstance(nullptr);

        }


        auto entryCollisionBox = entry->getCollisionRectangle();

        if (GeometryExtensions::rectIntersectsRect(*boundsCollisionBox, *entryCollisionBox)) {
            if (entry->getMappedInstance() == nullptr && !entry->getRespawnPreventionFlag()) {
				auto entity = entry->getCreateFunction()();
				
				assert(entity != nullptr);
				
                entry->setMappedInstance(entity);

                entry->getMappedInstance()->setEntry(entry);

				if(entry->getDefaultParent() == nullptr){
					_level->addChild(entry->getMappedInstance(), entry->getPriority());
				} else {
					entry->getDefaultParent()->addChild(entry->getMappedInstance(), entry->getPriority());
				}
				
                _level->getPhysicsWorld()->getEntities().push_back(entry->getMappedInstance());
            }


            if (entry->getMappedInstance() != nullptr) {
                auto instanceCollisionBox = entry->getMappedInstance()->getCollisionBox();

                if (!GeometryExtensions::rectIntersectsRect(inflatedBoundsCollisionBox, *instanceCollisionBox)) {
                    entry->setFinished(true);

                    entry->getMappedInstance()->onFinished();

					auto& entities = _level->getPhysicsWorld()->getEntities();
					auto it = std::find(entities.begin(), entities.end(), entry->getMappedInstance());

					_level->getPhysicsWorld()->getEntities().erase(it);
                    _level->getPhysicsWorld()->unregisterContact(entry->getMappedInstance());

                    entry->getMappedInstance()->removeFromParent();

                    entry->setMappedInstance(nullptr);
                }
                else {
                    entry->setRespawnPreventionFlag(true);
                }
            }

        }
        else {

            if (entry->getMappedInstance() != nullptr) {
                auto instanceCollisionBox = entry->getMappedInstance()->getCollisionBox();

                if (!GeometryExtensions::rectIntersectsRect(inflatedBoundsCollisionBox, *instanceCollisionBox)) {
                    entry->setFinished(true);
                    entry->setRespawnPreventionFlag(false);

                    entry->getMappedInstance()->onFinished();


					auto& entities = _level->getPhysicsWorld()->getEntities();
					auto it = std::find(entities.begin(), entities.end(), entry->getMappedInstance());

					_level->getPhysicsWorld()->getEntities().erase(it);
                    _level->getPhysicsWorld()->unregisterContact(entry->getMappedInstance());

                    entry->getMappedInstance()->removeFromParent();

                    entry->setMappedInstance(nullptr);
                }
            }
            else {
                entry->setRespawnPreventionFlag(false);
                entry->setFinished(false);

            }
           
        }

    }
}
