#include "ComponentPool.h"
#include "Core/Core.h"

namespace Flameberry {
    uint32_t typeCounter = 0;

    ComponentPool::ComponentPool(size_t component_size)
        : _ComponentSize(component_size), _EntityIdSet(MAX_ENTITIES, MAX_ENTITIES)
    {
        _Data = new char[_ComponentSize * MAX_ENTITIES];
    }

    void* ComponentPool::GetComponentAddress(const entity_handle& entity) const
    {
        if (entity.get() <= MAX_ENTITIES && _EntityIdSet.search(entity.get()) != -1)
            return _Data + _EntityIdSet.search(entity.get()) * _ComponentSize;
        if (entity.get() > MAX_ENTITIES)
            FL_ERROR("Component Pool out of memory", entity.get());
        return NULL;
    }

    void ComponentPool::Add(const entity_handle& entity) { _EntityIdSet.insert(entity.get()); }

    void ComponentPool::Remove(const entity_handle& entity)
    {
        auto value = _EntityIdSet[_EntityIdSet.size() - 1];
        void* _src = GetComponentAddress(entity_handle{ value });
        void* _dest = GetComponentAddress(entity);

        _EntityIdSet.remove(entity.get());

        memcpy(_dest, _src, _ComponentSize);
    }

    ComponentPool::~ComponentPool()
    {
        delete[] _Data;
    }
}
