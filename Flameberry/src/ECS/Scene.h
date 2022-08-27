#pragma once

#include <vector>
#include <memory>

#include "Core/Core.h"

#include "Entity.h"
#include "ComponentPool.h"

namespace Flameberry {
    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        Entity CreateEntity();
        void DestroyEntity(Entity& entity);

        template<typename T>
        T* AddComponent(const Entity& entity);
        template<typename T>
        T* GetComponent(const Entity& entity);
    private:
        std::vector<std::shared_ptr<ComponentPool>> m_ComponentPools;
        std::vector<Entity> m_Entities;
        std::vector<uint32_t> m_FreeEntities;
    };

    template<typename T>
    T* Scene::AddComponent(const Entity& entity)
    {
        if (!entity.GetValidity())
        {
            FL_WARN("Cannot add component to invalid entity");
            return NULL;
        }

        uint32_t componentTypeId = GetComponentTypeId<T>();

        // Check to see if component pool of type T exists
        if (componentTypeId >= m_ComponentPools.size())
        {
            // Resize Component pool vector
            m_ComponentPools.resize(componentTypeId + 1, nullptr);
            m_ComponentPools[componentTypeId] = std::make_shared<ComponentPool>(sizeof(T));
        }

        // Checking if the entity has the component of type T
        if (m_ComponentPools[componentTypeId]->GetComponentAddress(entity.entityId) != NULL)
        {
            FL_WARN("Attempted to assign the entity a component of type which already exists!");
            return NULL;
        }

        m_ComponentPools[componentTypeId]->AddEntityId(entity.entityId);
        void* componentAddress = m_ComponentPools[componentTypeId]->GetComponentAddress(entity.entityId);
        if (componentAddress == NULL)
        {
            FL_ERROR("Couldn't assign component to the entity with id: {0}", entity.entityId);
            return NULL;
        }
        componentAddress = (T*)(new(componentAddress) T());
        return static_cast<T*>(componentAddress);
    }

    template<typename T>
    T* Scene::GetComponent(const Entity& entity)
    {
        if (!entity.GetValidity())
        {
            FL_WARN("Cannot access component of invalid entity");
            return NULL;
        }

        uint32_t componentTypeId = GetComponentTypeId<T>();
        if (componentTypeId >= m_ComponentPools.size() && m_ComponentPools[componentTypeId]->GetComponentAddress(entity.entityId) == NULL)
        {
            // Component of T type doesn't exist
            FL_WARN("Attempted to access non-existing component with type id: {0} of the entity with id: {1}", componentTypeId, entity.entityId);
            return NULL;
        }
        return (T*)m_ComponentPools[componentTypeId]->GetComponentAddress(entity.entityId);
    }
}
