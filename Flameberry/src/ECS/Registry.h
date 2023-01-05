#pragma once

#include <vector>
#include <memory>

#include "Core/Core.h"
#include "ComponentPool.h"
#include "Component.h"

namespace Flameberry {
    template<typename... ComponentTypes>
    class SceneView;

    class Registry
    {
    public:
        Registry() = default;
        ~Registry() = default;

        entity_handle& CreateEntity();
        void DestroyEntity(entity_handle& entity);

        template <typename Fn>
        void each(Fn&& _Fn)
        {
            static_assert(std::is_invocable_v<Fn, entity_handle&>);
            for (auto& entity : m_Entities)
            {
                if (entity.is_valid())
                    _Fn(entity);
            }
        }
        // void each(const std::function<void(entity_handle& entity)>& perform_op);

        void Clear();

        template<typename T, typename... Args>
        T* AddComponent(const entity_handle& entity, Args... args);

        template<typename T>
        T* GetComponent(const entity_handle& entity) const;

        template<typename... ComponentTypes>
        std::tuple<ComponentTypes*...> Get(const entity_handle& entity) const;

        template<typename T>
        bool HasComponent(const entity_handle& entity) const;

        template<typename... ComponentTypes>
        bool Has(const entity_handle& entity) const;

        template<typename T>
        void RemoveComponent(const entity_handle& entity);

        template<typename T>
        bool DoesComponentPoolExist();

        template<typename T>
        inline std::shared_ptr<ComponentPool> GetComponentPool() const { return m_ComponentPools[GetComponentTypeId<T>()]; }

        template<typename... ComponentTypes>
        SceneView<ComponentTypes...> View();

        std::vector<entity_handle>& GetEntityVector() { return m_Entities; }
    private:
        std::vector<std::shared_ptr<ComponentPool>> m_ComponentPools;
        std::vector<entity_handle> m_Entities;
        std::vector<uint32_t> m_FreeEntities;

        friend class Scene;
    };

    template<typename T, typename... Args>
    T* Registry::AddComponent(const entity_handle& entity, Args... args)
    {
        FL_ASSERT(entity.is_valid(), "Failed to add component to the entity: INVALID_ENTITY!");
        uint32_t componentTypeId = GetComponentTypeId<T>();

        // Check to see if component pool of type T exists
        if (componentTypeId >= m_ComponentPools.size())
        {
            // Resize Component pool vector
            m_ComponentPools.resize(componentTypeId + 1, nullptr);
            m_ComponentPools[componentTypeId] = std::make_shared<ComponentPool>(sizeof(T));
        }

        // Checking if the entity has the component of type T
        FL_ASSERT(!m_ComponentPools[componentTypeId]->ContainsEntity(entity), "Attempted to assign the entity a component of type which already exists!");

        m_ComponentPools[componentTypeId]->Add(entity.get());
        void* componentAddress = m_ComponentPools[componentTypeId]->GetComponentAddress(entity);
        return (T*)(new(componentAddress) T(std::forward<Args>(args)...));
    }

    template<typename T>
    T* Registry::GetComponent(const entity_handle& entity) const
    {
        FL_ASSERT(HasComponent<T>(entity), "Entity does not have component!");
        T* ptr = static_cast<T*>(m_ComponentPools[GetComponentTypeId<T>()]->GetComponentAddress(entity));
        return ptr;
    }

    template<typename... ComponentTypes>
    std::tuple<ComponentTypes*...> Registry::Get(const entity_handle& entity) const
    {
        return std::make_tuple<ComponentTypes*...>(GetComponent<ComponentTypes>(entity)...);
    }

    template<typename T>
    void Registry::RemoveComponent(const entity_handle& entity)
    {
        FL_ASSERT(HasComponent<T>(entity), "Entity does not have component!");
        m_ComponentPools[GetComponentTypeId<T>()]->Remove(entity.get());
    }

    template<typename T>
    inline bool Registry::DoesComponentPoolExist()
    {
        uint32_t componentTypeID = GetComponentTypeId<T>();
        return m_ComponentPools.size() > componentTypeID && m_ComponentPools[componentTypeID].get() != nullptr;
    }

    template<typename T>
    bool Registry::HasComponent(const entity_handle& entity) const
    {
        FL_ASSERT(entity.is_valid(), "Attempted to check component existence from an invalid entity");
        uint32_t componentTypeId = GetComponentTypeId<T>();
        return componentTypeId < m_ComponentPools.size() && m_ComponentPools[componentTypeId]->ContainsEntity(entity);
    }

    template<typename... ComponentTypes>
    bool Registry::Has(const entity_handle& entity) const
    {
        FL_ASSERT(entity.is_valid(), "Attempted to check component existence from an invalid entity");
        uint32_t componentTypeIds[] = { GetComponentTypeId<ComponentTypes>() ... };
        bool hasComponents = true;
        for (const auto& id : componentTypeIds)
            hasComponents = hasComponents && (id < m_ComponentPools.size() && m_ComponentPools[id]->ContainsEntity(entity));
        return hasComponents;
    }
}

#include "SceneView.h"

namespace Flameberry {
    template<typename... ComponentTypes>
    SceneView<ComponentTypes...> Registry::View()
    {
        if (!m_ComponentPools.size())
        {
            FL_ERROR("Attempted to view registry with no component pools!");
            return SceneView<ComponentTypes...>(*this, NULL);
        }

        uint32_t componentIds[] = { GetComponentTypeId<ComponentTypes>() ... };
        int smallestPool = 0;
        for (const auto& id : componentIds)
        {
            if (m_ComponentPools.size() < id && m_ComponentPools[smallestPool]->size() > m_ComponentPools[id]->size() && m_ComponentPools[id]->size())
                smallestPool = id;
        }

        return SceneView<ComponentTypes...>(*this, m_ComponentPools[smallestPool]);
    }
}
