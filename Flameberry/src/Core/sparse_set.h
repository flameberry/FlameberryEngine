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

        pointer_type get() { return _ptr; }

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

    template<typename sparse_set>
    class sparse_set_const_iterator
    {
    public:
        using value_type = typename sparse_set::value_type;
        using pointer_type = value_type*;
        using reference_type = const value_type&;
    public:
        sparse_set_const_iterator(pointer_type ptr)
            : _ptr(ptr) {}

        sparse_set_const_iterator& operator++()
        {
            _ptr++;
            return *this;
        }

        sparse_set_const_iterator operator++(int)
        {
            sparse_set_const_iterator iterator = *this;
            ++(*this);
            return iterator;
        }

        sparse_set_const_iterator& operator--()
        {
            _ptr--;
            return *this;
        }

        sparse_set_const_iterator operator--(int)
        {
            sparse_set_const_iterator iterator = *this;
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

        bool operator==(const sparse_set_const_iterator& other)
        {
            return _ptr == other._ptr;
        }

        bool operator!=(const sparse_set_const_iterator& other)
        {
            return !((*this) == other);
        }
    private:
        pointer_type _ptr;
    };

    template<typename sparse_set>
    class sparse_set_reverse_iterator
    {
    public:
        using value_type = typename sparse_set::value_type;
        using pointer_type = value_type*;
        using reference_type = value_type&;
    public:
        sparse_set_reverse_iterator(pointer_type ptr)
            : _ptr(ptr) {}

        pointer_type get() { return _ptr; }

        sparse_set_reverse_iterator& operator++()
        {
            _ptr--;
            return *this;
        }

        sparse_set_reverse_iterator operator++(int)
        {
            sparse_set_reverse_iterator iterator = *this;
            --(*this);
            return iterator;
        }

        sparse_set_reverse_iterator& operator--()
        {
            _ptr++;
            return *this;
        }

        sparse_set_reverse_iterator operator--(int)
        {
            sparse_set_reverse_iterator iterator = *this;
            ++(*this);
            return iterator;
        }

        reference_type operator[](int index)
        {
            return *(_ptr - index);
        }

        pointer_type operator->()
        {
            return _ptr;
        }

        reference_type operator*()
        {
            return *_ptr;
        }

        bool operator==(const sparse_set_reverse_iterator& other)
        {
            return _ptr == other._ptr;
        }

        bool operator!=(const sparse_set_reverse_iterator& other)
        {
            return _ptr != other._ptr;
        }
    private:
        pointer_type _ptr;
    };

    template<typename sparse_set>
    class sparse_set_reverse_const_iterator
    {
    public:
        using value_type = typename sparse_set::value_type;
        using pointer_type = value_type*;
        using reference_type = const value_type&;
    public:
        sparse_set_reverse_const_iterator(pointer_type ptr)
            : _ptr(ptr) {}

        sparse_set_reverse_const_iterator& operator++()
        {
            _ptr++;
            return *this;
        }

        sparse_set_reverse_const_iterator operator++(int)
        {
            sparse_set_reverse_const_iterator iterator = *this;
            ++(*this);
            return iterator;
        }

        sparse_set_reverse_const_iterator& operator--()
        {
            _ptr--;
            return *this;
        }

        sparse_set_reverse_const_iterator operator--(int)
        {
            sparse_set_reverse_const_iterator iterator = *this;
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

        bool operator==(const sparse_set_reverse_const_iterator& other)
        {
            return _ptr == other._ptr;
        }

        bool operator!=(const sparse_set_reverse_const_iterator& other)
        {
            return _ptr != other._ptr;
        }
    private:
        pointer_type _ptr;
    };

    class sparse_set
    {
    public:
        using value_type = uint64_t;
        using iterator = sparse_set_iterator<sparse_set>;
        using const_iterator = sparse_set_const_iterator<sparse_set>;
        using reverse_iterator = sparse_set_reverse_iterator<sparse_set>;
        using reverse_const_iterator = sparse_set_reverse_const_iterator<sparse_set>;
    public:
        sparse_set(size_t max_val);
        ~sparse_set();
        void insert(value_type value);
        void remove(value_type value);
        value_type search(value_type value) const;
        void print();
        inline void clear() { _size = 0; }
        inline size_t size() const { return _size; }
        inline bool full() const { return _size == capacity; }
        inline bool empty() const { return _size == 0; }

        value_type operator[](size_t index) {
            return packed_data[index];
        }

        iterator begin() const { return iterator(packed_data); }
        iterator end() const { return iterator(packed_data + _size); }
        const_iterator cbegin() const { return const_iterator(packed_data); }
        const_iterator cend() const { return const_iterator(packed_data + _size); }
        reverse_iterator rbegin() const { return reverse_iterator(packed_data + _size - 1); }
        reverse_iterator rend() const { return reverse_iterator(packed_data - 1); }
        reverse_const_iterator crbegin() const { return reverse_const_iterator(packed_data + _size - 1); }
        reverse_const_iterator crend() const { return reverse_const_iterator(packed_data - 1); }
    private:
        value_type* sparse_data;
        value_type* packed_data;
        value_type max_value;
        size_t capacity, _size;
    };
}