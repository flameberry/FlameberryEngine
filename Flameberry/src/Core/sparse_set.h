#pragma once

#include <iostream>

namespace utils {
    class sparse_set {
    public:
        sparse_set(size_t capacity, int32_t max_val);
        ~sparse_set();
        void insert(int32_t value);
        void remove(int32_t value);
        int32_t search(int32_t value);
        inline void clear() { m_CurrentNumberOfElements = 0; }
        void print();
    private:
        int32_t* m_SparseArray;
        int32_t* m_DenseArray;
        int32_t m_MaxValue;
        size_t m_Capacity, m_CurrentNumberOfElements;
    };
}