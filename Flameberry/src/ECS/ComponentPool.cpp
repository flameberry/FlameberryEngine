#include "ComponentPool.h"
#include "Core/Core.h"

namespace Flameberry {
    uint32_t typeCounter = 0;

    ComponentPool::ComponentPool(size_t componentSize)
        : m_ComponentSize(componentSize), m_ComponentSet(MAX_ENTITIES, MAX_ENTITIES)
    {
        m_ComponentData = new char[m_ComponentSize * MAX_ENTITIES];
    }

    void* ComponentPool::GetComponentAddress(uint32_t entityId)
    {
        if (entityId <= MAX_ENTITIES && m_ComponentSet.search(entityId) != -1)
            return m_ComponentData + m_ComponentSet.search(entityId) * m_ComponentSize;
        if (entityId > MAX_ENTITIES)
            FL_WARN("Component Pool out of memory", entityId);
        return NULL;
    }

    void ComponentPool::AddEntityId(uint32_t entityId) { m_ComponentSet.insert(entityId); }
    void ComponentPool::RemoveEntityId(uint32_t entityId) { m_ComponentSet.remove(entityId); }

    ComponentPool::~ComponentPool()
    {
        delete[] m_ComponentData;
    }
}
