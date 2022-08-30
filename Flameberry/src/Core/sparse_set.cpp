#include "sparse_set.h"

namespace utils {
    sparse_set::sparse_set(size_t capacity, int32_t max_val)
        : capacity(capacity), max_value(max_val), _size(0)
    {
        sparse_data = new int32_t[max_val + 1];
        packed_data = new int32_t[capacity];
    }

    void sparse_set::insert(int32_t value)
    {
        if (value > max_value || _size >= capacity || search(value) != -1)
            return;

        packed_data[_size] = value;
        sparse_data[value] = _size;
        _size++;
    }

    int32_t sparse_set::search(int32_t value) const
    {
        if (value <= max_value && sparse_data[value] < _size && packed_data[sparse_data[value]] == value)
            return sparse_data[value];
        return -1;
    }

    void sparse_set::remove(int32_t value)
    {
        if (search(value) == -1)
            return;
        size_t temp = packed_data[_size - 1];
        packed_data[sparse_data[value]] = temp;
        sparse_data[temp] = sparse_data[value];
        _size--;
    }

    void sparse_set::print()
    {
        for (size_t i = 0; i < _size; i++)
            printf("%d ", packed_data[i]);
        printf("\n");
    }

    sparse_set::~sparse_set()
    {
        delete[] sparse_data;
        delete[] packed_data;
    }
}
