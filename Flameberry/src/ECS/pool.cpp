#include "pool.h"

namespace Flameberry {
    component_pool::component_pool(size_t comp_size, size_t capacity)
        : m_ComponentSize(comp_size), m_Capacity(capacity), m_EntitySet(capacity - 1)
    {
    }

    component_pool::~component_pool()
    {
        if (m_Data)
            delete[] m_Data;
    }

    void component_pool::allocate(size_t comp_size, size_t capacity)
    {
        FL_ASSERT((capacity || m_Capacity) && (comp_size || m_ComponentSize), "Failed to allocate component_pool memory: Capacity is 0!");
        if (capacity)
            m_Capacity = capacity;
        if (comp_size)
            m_ComponentSize = comp_size;
        m_Data = new char[m_Capacity * m_ComponentSize];
    }

    void* component_pool::get(const entity_handle& entity) const
    {
        if (utils::sparse_set::value_type found_handle = m_EntitySet.search(entity.get()); entity.get() < m_Capacity && found_handle != -1)
            return m_Data + found_handle;
        return nullptr;
    }

    void component_pool::remove(const entity_handle& entity)
    {
        auto value = m_EntitySet[m_EntitySet.size() - 1];
        void* _src = get(entity_handle{ value });
        void* _dest = get(entity);

        m_EntitySet.remove(entity.get());
        memcpy(_dest, _src, m_ComponentSize);
    }
}