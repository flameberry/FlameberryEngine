#include "sparse_set.h"

namespace utils {
    sparse_set::sparse_set(size_t capacity, int32_t max_val)
        : m_Capacity(capacity), m_MaxValue(max_val), m_CurrentNumberOfElements(0)
    {
        m_SparseArray = new int32_t[max_val + 1];
        m_DenseArray = new int32_t[capacity];
    }

    void sparse_set::insert(int32_t value)
    {
        if (value > m_MaxValue || m_CurrentNumberOfElements >= m_Capacity || search(value) != -1)
            return;

        m_DenseArray[m_CurrentNumberOfElements] = value;
        m_SparseArray[value] = m_CurrentNumberOfElements;
        m_CurrentNumberOfElements++;
    }

    int32_t sparse_set::search(int32_t value)
    {
        if (value <= m_MaxValue && m_SparseArray[value] < m_CurrentNumberOfElements && m_DenseArray[m_SparseArray[value]] == value)
            return m_SparseArray[value];
        return -1;
    }

    void sparse_set::remove(int32_t value)
    {
        if (search(value) == -1)
            return;
        size_t temp = m_DenseArray[m_CurrentNumberOfElements - 1];
        m_DenseArray[m_SparseArray[value]] = temp;
        m_SparseArray[temp] = m_SparseArray[value];
        m_CurrentNumberOfElements--;
    }

    void sparse_set::print()
    {
        for (size_t i = 0; i < m_CurrentNumberOfElements; i++)
            printf("%d ", m_DenseArray[i]);
        printf("\n");
    }

    sparse_set::~sparse_set()
    {
        delete[] m_SparseArray;
        delete[] m_DenseArray;
    }
}
