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

        void Add(uint32_t entityId);
        void Remove(uint32_t entityId);

        void* GetComponentAddress(uint32_t entityId) const;
        utils::sparse_set& GetEntityIdSet() { return _EntityIdSet; }
        size_t size() const { return _EntityIdSet.size(); }
    private:
        char* _Data = nullptr;
        utils::sparse_set _EntityIdSet;
        size_t _ComponentSize = 0;
    };
}
