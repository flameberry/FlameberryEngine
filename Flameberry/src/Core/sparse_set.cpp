#include "sparse_set.h"

namespace utils {
    sparse_set::sparse_set(size_t capacity, value_type max_val)
        : capacity(capacity), max_value(max_val), _size(0)
    {
        sparse_data = new value_type[max_val + 1];
        packed_data = new value_type[capacity];
    }

    void sparse_set::insert(value_type value)
    {
        if (value > max_value || _size >= capacity || search(value) != -1)
            return;

        packed_data[_size] = value;
        sparse_data[value] = _size;
        _size++;
    }

    sparse_set::value_type sparse_set::search(value_type value) const
    {
        if (value <= max_value && sparse_data[value] < _size && packed_data[sparse_data[value]] == value)
            return sparse_data[value];
        return -1;
    }

    void sparse_set::remove(value_type value)
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
            printf("%llu ", packed_data[i]);
        printf("\n");
    }

    sparse_set::~sparse_set()
    {
        delete[] sparse_data;
        delete[] packed_data;
    }
}
