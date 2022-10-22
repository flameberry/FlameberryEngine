#pragma once

#include "Core/Core.h"
#include "Core/sparse_set.h"

#include "Entity.h"

namespace Flameberry {
    class component_pool
    {
    public:
        component_pool();
        component_pool(size_t comp_size, size_t capacity);
        ~component_pool();

        void allocate(size_t comp_size = 0, size_t capacity = 0);
        size_t capacity() const { return m_Capacity; }
        size_t size() const { return m_EntitySet.size(); }
        size_t comp_size() const { return m_ComponentSize; }

        void* get(const entity_handle& entity) const;
        utils::sparse_set& get_entity_set() { return m_EntitySet; }

        void add(const entity_handle& entity) { m_EntitySet.insert(entity.get()); }
        void remove(const entity_handle& entity);
    private:
        size_t m_ComponentSize, m_Capacity;
        char* m_Data = nullptr;
        utils::sparse_set m_EntitySet;
    };
}
