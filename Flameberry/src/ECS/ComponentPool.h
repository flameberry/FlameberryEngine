#pragma once

#include "Core/Core.h"
#include "Core/sparse_set.h"

#include "Entity.h"

namespace Flameberry {
    class ComponentPool
    {
    public:
        ComponentPool(size_t componentSize);
        ~ComponentPool();

        void Add(const entity_handle& entity);
        void Remove(const entity_handle& entity);

        const void* Get() const { return _Data; }
        void* GetComponentAddress(const entity_handle& entity) const;
        bool ContainsEntity(const entity_handle& entity) const { return entity.get() <= MAX_ENTITIES && _EntityIdSet.search(entity.get()) != -1; }
        utils::sparse_set& GetEntityIdSet() { return _EntityIdSet; }
        size_t size() const { return _EntityIdSet.size(); }
    private:
        char* _Data = nullptr;
        utils::sparse_set _EntityIdSet;
        size_t _ComponentSize = 0;
    };
}
