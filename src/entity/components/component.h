#ifndef ANIM_ENTITY_COMPONENT_COMPONENT_H
#define ANIM_ENTITY_COMPONENT_COMPONENT_H

#include <memory>
#include <vector>

namespace anim
{
	class Entity;

    typedef int *TypeID;
    class Component
    {
    public:
        virtual ~Component() = default;
        virtual TypeID get_type() const = 0;
        virtual void update(std::shared_ptr<Entity> entity) = 0;
		
		virtual std::shared_ptr<Component> clone() = 0;
    };

    template <class T>
    class ComponentBase : public Component
    {
    public:
        virtual ~ComponentBase() = default;
        virtual void update(std::shared_ptr<Entity> entity) override{};
        static TypeID type;
        TypeID get_type() const override { return T::type; }
    };
    template <typename T>
    TypeID ComponentBase<T>::type((TypeID)&T::type);
}
#endif
