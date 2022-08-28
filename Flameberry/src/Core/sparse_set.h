#pragma once

#include <iostream>

namespace utils {
    template<typename sparse_set>
    class sparse_set_iterator
    {
    public:
        using value_type = typename sparse_set::value_type;
        using pointer_type = value_type*;
        using reference_type = value_type&;
    public:
        sparse_set_iterator(pointer_type ptr)
            : _ptr(ptr) {}

        sparse_set_iterator& operator++()
        {
            _ptr++;
            return *this;
        }

        sparse_set_iterator operator++(int)
        {
            sparse_set_iterator iterator = *this;
            ++(*this);
            return iterator;
        }

        sparse_set_iterator& operator--()
        {
            _ptr--;
            return *this;
        }

        sparse_set_iterator operator--(int)
        {
            sparse_set_iterator iterator = *this;
            --(*this);
            return iterator;
        }

        reference_type operator[](int index)
        {
            return *(_ptr + index);
        }

        pointer_type operator->()
        {
            return _ptr;
        }

        reference_type operator*()
        {
            return *_ptr;
        }

        bool operator==(const sparse_set_iterator& other)
        {
            return _ptr == other._ptr;
        }

        bool operator!=(const sparse_set_iterator& other)
        {
            return _ptr != other._ptr;
        }
    private:
        pointer_type _ptr;
    };

    class sparse_set
    {
    public:
        using value_type = int32_t;
        using iterator = sparse_set_iterator<sparse_set>;
    public:
        sparse_set(size_t capacity, int32_t max_val);
        ~sparse_set();
        void insert(int32_t value);
        void remove(int32_t value);
        int32_t search(int32_t value);
        void print();
        inline void clear() { _size = 0; }
        inline size_t size() const { return _size; }
        inline bool full() const { return _size == capacity; }
        inline bool empty() const { return _size == 0; }
        // inline const int32_t* data() const { return packed_data; }

        iterator begin() { return iterator(packed_data); }
        iterator end() { return iterator(packed_data + _size); }
    private:
        int32_t* sparse_data;
        int32_t* packed_data;
        int32_t max_value;
        size_t capacity, _size;
    };
}