#pragma once

#include <vector>
#include <map>
#include <memory>
#include <tuple>

#include <glm/glm.hpp>

namespace anim{
class Entity;
}


namespace windy {
enum class Contact {
	Up,
	Down,
	Left,
	Right
};

struct CollisionContact {
	Contact type;
	bool contact;
	
	std::shared_ptr<anim::Entity> entity;
};
}


namespace physics {
    class PhysicsWorld {
    public:
        static std::shared_ptr<PhysicsWorld> create();

        virtual bool init();

        void setUnderwater(bool underwater);

        virtual void update(float dt);

        static void alignCollisions(std::shared_ptr<anim::Entity> entity, std::shared_ptr<anim::Entity> landscapeEntity, bool clearContacts = false);

        void addEntity(std::shared_ptr<anim::Entity> entity);
        void removeEntity(std::shared_ptr<anim::Entity> entity);

        std::vector<std::shared_ptr<anim::Entity>>& getEntities() { return _entities; }


        void unregisterContact(std::shared_ptr<anim::Entity> a, std::shared_ptr<anim::Entity> b);
        void unregisterContact(std::shared_ptr<anim::Entity> a);

        void resetContactEventCollisions();

        void setGravity(float gravity) { _gravity = gravity; }
        float getGravity() { return _gravity; }

    private:
        float _maxFallSpeed;

        float _gravity;


        std::vector<std::shared_ptr<anim::Entity>> _entities;

        long long _contactEventCollisionIndex;
        std::vector<std::pair<long long, std::pair<std::shared_ptr<anim::Entity>, std::shared_ptr<anim::Entity>>>> _contactEventCollisions;

    };

}
