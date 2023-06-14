#pragma once

#include <stdint.h>
#include <vector>

#include "Core/Core.h"

#define FL_TYPE_NAME(type) #type

/* This is a header only implementation of the ecs system */
/* Formatting for this file includes brackets on the same line as the function to keep it compact */
namespace fbentt {
    class TypeCounter {
        inline static uint32_t typeCounter = 0;
        template<typename Type> friend uint32_t type_id();
    };

    template<typename Type> uint32_t type_id() {
        static uint32_t componentCounter = TypeCounter::typeCounter++;
        return componentCounter;
    }

    template<typename T, typename... Types> decltype(auto) get_first_of_variadic_template() {
        return T();
    }

    template<typename Type> class sparse_set {
    public:
        void add(Type value) {
            // Check if value exists in set
            if (int index = find(value); index != -1) {
                packed[index] = value;
                return;
            }
            else if (value >= sparse.size()) {
                sparse.resize(value + 1);
            }
            sparse[value] = (Type)packed.size();
            packed.emplace_back(value);
        }

        int remove(Type value) {
            if (value >= sparse.size() || sparse[value] >= packed.size()) {
                FL_WARN("Attempted to remove element '{0}' from sparse set that didn't belong to set!", value);
                return -1;
            }
            // copying the last element of packed array to the removed element location
            Type last = packed.back();
            packed[sparse[value]] = last;
            sparse[last] = sparse[value];
            packed.pop_back();
            return sparse[value];
        }

        /// @brief 
        /// @param value The value to be searched in the sparse set
        /// @return Returns the location of the value found in packed array, or returns -1 if the value is not found
        int find(Type value) const {
            if (value >= sparse.size() || sparse[value] >= packed.size() || packed[sparse[value]] != value) {
                return -1;
            }
            return sparse[value];
        }

        inline uint32_t size() const { return (uint32_t)packed.size(); }
        inline bool empty() const { return packed.empty(); }

        inline void clear() {
            packed.clear();
            sparse.clear();
        }

        inline Type operator[](size_t index) const {
            return packed[index];
        }

        // Debug
#ifdef FL_DEBUG
        void print() const {
            for (const auto& element : packed)
                std::cout << element << "\t";
            std::cout << "\n";
        }
#endif
    private:
        std::vector<Type> packed, sparse;
    };

    struct null_t;
    extern null_t null;

    class entity_handle {
    public:
        using handle_type = uint32_t;
    public:
        entity_handle();
        entity_handle(handle_type handle) : handle(handle), validity(true) {}
        entity_handle(handle_type handle, bool validity) : handle(handle), validity(validity) {}

        inline operator handle_type() const { return handle; }

        inline bool operator==(const entity_handle& entity) const {
            return this->handle == entity.handle && this->validity == entity.validity;
        }
        inline bool operator!=(const entity_handle& entity) const {
            return !(*this == entity);
        }
    private:
        handle_type handle;
        bool validity;

        friend class registry;
    };

    struct null_t {
        inline operator entity_handle() const {
            return entity_handle(-1, false);
        }

        inline constexpr bool operator==(const null_t& other) const {
            return true;
        }

        inline constexpr bool operator!=(const null_t& other) const {
            return false;
        }

        inline constexpr bool operator==(const entity_handle& other) const {
            return false;
        }

        inline constexpr bool operator!=(const entity_handle& other) const {
            return true;
        }
    };

    template<typename Type> class pool_handler {
    public:
        template<typename... Args> Type& emplace(Args... args) {
            auto& component = component_buffer.emplace_back(std::forward<Args>(args)...);
            return component;
        }

        Type& get(uint32_t index) {
            return component_buffer[index];
        }

        void remove(uint32_t index) {
            component_buffer[index] = component_buffer.back();
            component_buffer.pop_back();
        }
    private:
        std::vector<Type> component_buffer;
    };

    struct pool_data {
        sparse_set<uint32_t> entity_set;
        std::shared_ptr<void> handler{ nullptr };
        void (*remove)(const pool_data& pool, uint32_t index);
    };

    class registry {
    public:
        template<typename... Type> class registry_view {
        public:
            template<typename... i_Type> class iterator {
            public:
                iterator(const registry* reg, const pool_data* pool, uint32_t index) : ref_registry(reg), ref_pool(pool), index(index) {}

                iterator<i_Type...>& operator++() {
                    while (++index < ref_pool->entity_set.size() && !ref_registry->has<i_Type...>(ref_registry->entities[ref_pool->entity_set[index]]));
                    return *this;
                }

                iterator<i_Type...> operator++(int) {
                    auto it = *this;
                    ++(*this);
                    return it;
                }

                entity_handle operator*() {
                    return ref_registry->entities[ref_pool->entity_set[index]];
                }

                bool operator==(const iterator<i_Type...>& it) {
                    return this->index == it.index;
                }

                bool operator!=(const iterator<i_Type...>& it) {
                    return !(*this == it);
                }
            private:
                const registry* ref_registry;
                const pool_data* ref_pool;
                uint32_t index;
            };
        public:
            registry_view() : begin_index(0), end_index(0) {}
            registry_view(const registry* reg, const pool_data* pool) : ref_registry(reg), ref_pool(pool), begin_index(0), end_index(0) {
                while (begin_index < pool->entity_set.size() && !reg->has<Type...>(reg->entities[pool->entity_set[begin_index]]))
                    begin_index++;
                end_index = pool->entity_set.size();
            }

            iterator<Type...> begin() {
                return iterator<Type...>(ref_registry, ref_pool, begin_index);
            }

            iterator<Type...> end() {
                return iterator<Type...>(ref_registry, ref_pool, end_index);
            }
        private:
            const registry* ref_registry = nullptr;
            const pool_data* ref_pool = nullptr;
            int begin_index, end_index;
        };
    public:
        /// @brief: Iterates over all entities in the scene
        /// @param _Fn: A function with a param of type `ecs::entity_handle&` which represents the current entity being iterated
        template<typename Fn> void each(Fn&& _Fn) {
            static_assert(std::is_invocable_v<Fn, entity_handle&>);
            for (auto& entity : entities) {
                if (entity != null) {
                    _Fn(entity);
                }
            }
        }

        template<typename... Type> registry_view<Type...> view() {
            static_assert(sizeof...(Type) > 0);

            uint32_t typeIDs[] = { type_id<Type>()... };
            uint32_t smallestPoolIndex = 0;
            uint32_t smallestPoolSize = UINT32_MAX;

            for (const auto& typeID : typeIDs) {
                if (typeID >= pools.size()) {
                    return registry_view<Type...>();
                }
                if (uint32_t poolSize = pools[typeID].entity_set.size(); poolSize < smallestPoolSize) {
                    smallestPoolSize = poolSize;
                    smallestPoolIndex = typeID;
                }
            }
            return registry_view<Type...>(this, &pools[smallestPoolIndex]);
        }

        entity_handle create() {
            if (!free_entities.empty()) {
                uint32_t handle = (uint32_t)free_entities.back();
                entities[handle] = entity_handle(handle, true);
                free_entities.pop_back();
                return entities[handle];
            }
            return entities.emplace_back(entities.size(), true);
        }

        void destroy(entity_handle& entity) {
            FL_ASSERT(entity < entities.size() && entities[(uint32_t)entity] != null, "Attempted to destroy invalid entity!");

            for (auto& pool : pools) {
                if (pool.entity_set.find((uint32_t)entity) != -1) {
                    int index = pool.entity_set.remove((uint32_t)entity);
                    pool.remove(pool, index);
                }
            }
            free_entities.emplace_back((uint32_t)entity);
            entities[(uint32_t)entity] = null;
            entity = null;
        }

        template<typename Type, typename... Args> Type& emplace(const entity_handle& entity, Args... args) {
            FL_ASSERT(entity < entities.size() && entities[(uint32_t)entity] != null, "Attempted to emplace component to an invalid entity!");

            uint32_t typeID = type_id<Type>();
            if (pools.size() <= typeID) {
                pools.resize(typeID + 1);
                auto& pool = pools.back();
                pool.handler = std::make_shared<pool_handler<Type>>();
            }
            else if (pools[typeID].handler == nullptr) {
                pools[typeID].handler = std::make_shared<pool_handler<Type>>();
            }
            else if (pools[typeID].entity_set.find(entity) != -1) {
                FL_ERROR("Failed to emplace component of type: {0} to the entity: {1}: Entity already has component!", FL_TYPE_NAME(Type), (uint32_t)entity);
                FL_DEBUGBREAK();
            }
            pools[typeID].entity_set.add(entity.handle);
            pools[typeID].remove = [](const pool_data& pool, uint32_t index) {
                auto& handler = (*((pool_handler<Type>*)pool.handler.get()));
                handler.remove(index);
                };
            auto& handler = (*((pool_handler<Type>*)pools[typeID].handler.get()));
            return handler.emplace(std::forward<Args>(args)...);
        }

        template<typename... Type> decltype(auto) get(const entity_handle& entity) const {
            FL_ASSERT(entity < entities.size() && entities[(uint32_t)entity] != null, "Attempted to get component of invalid entity!");

            if constexpr (sizeof...(Type) == 1) {
                using ComponentType = decltype((*get_first_of_variadic_template<Type...>)());
                uint32_t typeID = type_id<ComponentType>();
                if (pools.size() <= typeID
                    || pools[typeID].handler == nullptr
                    || pools[typeID].entity_set.empty()
                    || pools[typeID].entity_set.find(entity.handle) == -1
                    ) {
                    FL_ERROR("Failed to get component of type: {0} of entity: {1}", FL_TYPE_NAME(ComponentType), entity.handle);
                    FL_DEBUGBREAK();
                }
                else {
                    int index = pools[typeID].entity_set.find(entity.handle);
                    auto& handler = (*((pool_handler<ComponentType>*)pools[typeID].handler.get()));
                    return static_cast<ComponentType&>(handler.get(index));
                }
            }
            else {
                return std::make_tuple(get<Type>(entity)...);
            }
        }

        template<typename... Type> bool has(const entity_handle& entity) const {
            FL_ASSERT(entity < entities.size() && entities[(uint32_t)entity] != null, "Failed to check existence of component of type: {0} invalid entity!", FL_TYPE_NAME(Type));

            if constexpr (sizeof...(Type) == 1) {
                using ComponentType = decltype((*get_first_of_variadic_template<Type...>)());
                uint32_t typeID = type_id<ComponentType>();
                if (pools.size() <= typeID
                    || pools[typeID].handler == nullptr
                    || pools[typeID].entity_set.empty()
                    || pools[typeID].entity_set.find(entity.handle) == -1
                    ) {
                    return false;
                }
                return true;
            }
            else {
                return (has<Type>(entity) && ...);
            }
        }

        void clear() {
            pools.clear();
            entities.clear();
            free_entities.clear();
        }
    private:
        std::vector<pool_data> pools;
        std::vector<entity_handle> entities;
        std::vector<uint32_t> free_entities;
    };
}
