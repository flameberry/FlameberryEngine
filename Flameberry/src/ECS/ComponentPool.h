#pragma once

#include "Core/sparse_set.h"
#include "Entity.h"

namespace Flameberry {
    class ComponentPool
    {
    public:
        ComponentPool(size_t componentSize);
        ~ComponentPool();

        void AddEntityId(uint32_t entityId);
        void RemoveEntityId(uint32_t entityId);
        void* GetComponentAddress(uint32_t entityId);
    private:
        char* m_ComponentData = nullptr;
        size_t m_ComponentSize = 0;
        utils::sparse_set m_ComponentSet;
    };
}
