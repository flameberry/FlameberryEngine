#include "Registry.h"
#include "Core/Core.h"

namespace Flameberry {
    entity_handle Registry::CreateEntity()
    {
        if (m_FreeEntities.size())
        {
            uint64_t id = m_FreeEntities.back();
            m_Entities[id] = entity_handle{ id };
            m_FreeEntities.pop_back();
            return entity_handle{ id };
        }
        m_Entities.emplace_back(m_Entities.size());
        return m_Entities.back();
    }

    void Registry::each(std::function<void(entity_handle& entity)> perform_op)
    {
        for (auto& entity : m_Entities)
        {
            if (entity.is_valid())
                perform_op(entity);
        }
    }

    void Registry::DestroyEntity(entity_handle& entity)
    {
        for (auto& commandPool : m_ComponentPools)
        {
            if (commandPool->GetComponentAddress(entity.get()))
                commandPool->Remove(entity.get());
        }
        m_Entities[entity.get()].set_validity(false);
        entity.set_validity(false);
        m_FreeEntities.push_back(entity.get());
    }
}
