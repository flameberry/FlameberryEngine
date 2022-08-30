#include "ComponentPool.h"
#include "Core/Core.h"

namespace Flameberry {
    uint32_t typeCounter = 0;

    ComponentPool::ComponentPool(size_t component_size)
        : _ComponentSize(component_size), _EntityIdSet(MAX_ENTITIES, MAX_ENTITIES)
    {
        _Data = new char[component_size * MAX_ENTITIES];
    }

    void* ComponentPool::GetComponentAddress(uint32_t entityId) const
    {
        if (entityId <= MAX_ENTITIES && _EntityIdSet.search(entityId) != -1)
            return _Data + _EntityIdSet.search(entityId) * _ComponentSize;
        if (entityId > MAX_ENTITIES)
            FL_WARN("Component Pool out of memory", entityId);
        return NULL;
    }

    void ComponentPool::Add(uint32_t entityId) { _EntityIdSet.insert(entityId); }
    void ComponentPool::Remove(uint32_t entityId) { _EntityIdSet.remove(entityId); }

    ComponentPool::~ComponentPool()
    {
        delete[] _Data;
    }
}
