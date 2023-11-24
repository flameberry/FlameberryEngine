#pragma once

#include <iostream>
#include <stdint.h>
#include <vector>

#include "Core/Core.h"

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
                FBY_WARN("Attempted to remove element '{}' from sparse set that didn't belong to set!", value);
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
#ifdef FBY_DEBUG
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

    /// @brief Contains a 64 bit integer -> Layout of handle: | 32-bit index | 31-bit version | 1-bit validity |
    class entity {
    public:
        using handle_type = std::uint64_t;
        using version_type = std::uint32_t;
    public:
        entity() : handle(0xFFFFFFFF00000000) {}
        entity(handle_type handle) : handle(handle) {}
        entity(const entity& entity) : handle(entity.handle) {}

        inline operator handle_type() const { return handle; }

        inline void operator=(entity entity) {
            this->handle = entity.handle;
        }

        inline void operator=(handle_type handle) {
            this->handle = handle;
        }

        inline constexpr void operator=(const null_t& n) {
            this->handle = 0xFFFFFFFF00000000;
        }

        inline bool operator==(const entity& entity) const {
            return this->handle == entity.handle;
        }

        inline bool operator!=(const entity& entity) const {
            return !(*this == entity);
        }
    private:
        handle_type handle;
    };

    inline entity to_handle(uint32_t index, entity::version_type version, bool validity) {
        return (entity::handle_type(index) << 32) | (entity::handle_type(version) << 1) | validity;
    }

    inline uint32_t to_index(entity handle) {
        return static_cast<uint32_t>(handle >> 32);
    }

    inline entity::version_type to_version(entity handle) {
        return static_cast<entity::version_type>(handle) >> 1;
    }

    inline bool is_valid(entity handle) {
        return handle & 0x1;
    }

    struct null_t {
        inline operator entity() const {
            return 0xFFFFFFFF00000000;
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
        void (*copy_handler_data)(const pool_data& src, pool_data& dest);

        pool_data() = default;
        explicit pool_data(const pool_data& pool)
            : entity_set(pool.entity_set), copy_handler_data(pool.copy_handler_data), remove(pool.remove)
        {
            if (pool.copy_handler_data != NULL) {
                // TODO: Check if this needs optimisation, currently the handler data is only copied upon loading the first scene and pressing the play button
                pool.copy_handler_data(pool, *this);
                FBY_LOG("Copied ecs component pool handler data!");
            }
        }
    };

    class registry {
    public:
        template<typename... Type> class registry_view {
        public:
            template<typename... iter_type> class iterator {
            public:
                iterator(const registry* reg, const pool_data* pool, uint32_t index) : ref_registry(reg), ref_pool(pool), index(index) {}

                iterator<iter_type...>& operator++() {
                    while (++index < ref_pool->entity_set.size() && !ref_registry->has<iter_type...>(ref_registry->entities[ref_pool->entity_set[index]]));
                    return *this;
                }

                iterator<iter_type...> operator++(int) {
                    auto it = *this;
                    ++(*this);
                    return it;
                }

                entity operator*() {
                    return ref_registry->entities[ref_pool->entity_set[index]];
                }

                bool operator==(const iterator<iter_type...>& it) {
                    return this->index == it.index;
                }

                bool operator!=(const iterator<iter_type...>& it) {
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
        entity get_entity_at_index(uint32_t index)
        {
            FBY_ASSERT(index < entities.size() && is_valid(entities[index]), "Failed to get entity at index: Invalid index/entity!");
            return entities[index];
        }

        /// @brief: Iterates over all entities in the scene
        /// @param _Fn: A function with a param of type `const ecs::entity&` which represents the current entity being iterated
        template<typename Fn> void for_each(Fn&& _Fn) {
            static_assert(std::is_invocable_v<Fn, entity>);
            for (auto entity : entities) {
                if (is_valid(entity)) {
                    _Fn(entity);
                }
            }
        }

        /// @brief: Iterates over all components in the scene
        /// @param _Fn: A function with a param of type `void*` which represents the current component being iterated
        /// And `uint32_t` which gives the typeID of the component
        template<typename Fn> void for_each_component(entity entity, Fn&& _Fn) {
            FBY_ASSERT(0, "for_each_component() Not Yet Implemented!");

            static_assert(std::is_invocable_v<Fn, void*, uint32_t>);

            FBY_ASSERT(entity != null, "Failed to iterate over components of entity: Entity is null!");

            const uint32_t index = to_index(entity);
            const uint32_t version = to_version(entities[index]);

            FBY_ASSERT(index < entities.size() && version == to_version(entity), "Failed to iterate over components of entity: Invalid handle!");

            uint32_t typeID = 0;
            for (auto& pool : pools) {
                if (int set_index = pool.entity_set.find(index); set_index != -1) {
                    // auto& handler = (*((pool_handler<Type>*)pool.handler.get()));
                    // _Fn(&handler.get(set_index), typeID);
                }
                typeID++;
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

        entity create() {
            if (!free_entities.empty()) {
                const uint32_t freeEntityIndex = free_entities.back();
                const uint32_t version = to_version(entities[freeEntityIndex]);
                entities[freeEntityIndex] = to_handle(freeEntityIndex, version + 1, true);

                free_entities.pop_back();
                return entities[freeEntityIndex];
            }
            return entities.emplace_back(to_handle(entities.size(), 0, true));
        }

        void destroy(entity entity) {
            FBY_ASSERT(entity != null, "Failed to destroy entity: Entity is null!");

            const uint32_t index = to_index(entity);
            const uint32_t version = to_version(entities[index]);

            FBY_ASSERT(index < entities.size() && version == to_version(entity), "Failed to delete entity: Invalid handle!");

            for (auto& pool : pools) {
                if (pool.entity_set.find(index) != -1) {
                    int set_index = pool.entity_set.remove(index);
                    pool.remove(pool, set_index);
                }
            }
            free_entities.emplace_back(index);
            entities[index] = entities[index] & 0xFFFFFFFFFFFFFFFE;
        }

        template<typename Type, typename... Args> Type& emplace(const entity& entity, Args... args) {
            FBY_ASSERT(entity != null, "Failed to emplace component: Entity is null!");
            const uint32_t index = to_index(entity);
            const uint32_t version = to_version(entities[index]);
            FBY_ASSERT(index < entities.size() && version == to_version(entity), "Failed to emplace component: Invalid/Outdated handle!");

            const uint32_t typeID = type_id<Type>();
            if (pools.size() <= typeID) {
                pools.resize(typeID + 1);
                auto& pool = pools.back();
                pool.handler = std::make_shared<pool_handler<Type>>();
            }
            else if (pools[typeID].handler == nullptr) {
                pools[typeID].handler = std::make_shared<pool_handler<Type>>();
            }
            else if (pools[typeID].entity_set.find(index) != -1) {
                FBY_ERROR("Failed to emplace component: Entity already has component!");
                FBY_DEBUGBREAK();
            }
            pools[typeID].entity_set.add(index);

            if (pools[typeID].remove == NULL) {
                pools[typeID].remove = [](const pool_data& pool, uint32_t index)
                    {
                        auto& handler = (*((pool_handler<Type>*)pool.handler.get()));
                        handler.remove(index);
                    };
            }

            if (pools[typeID].copy_handler_data == NULL) {
                pools[typeID].copy_handler_data = [](const pool_data& src, pool_data& dest)
                    {
                        if (src.handler)
                        {
                            auto& handler = (*((pool_handler<Type>*)src.handler.get()));
                            dest.handler = std::make_shared<pool_handler<Type>>(handler);
                        }
                    };
            }

            auto& handler = (*((pool_handler<Type>*)pools[typeID].handler.get()));
            return handler.emplace(std::forward<Args>(args)...);
        }

        template<typename... Type> decltype(auto) try_get(const entity& entity) const {
            if constexpr (sizeof...(Type) == 1) {
                const uint32_t index = to_index(entity);

                using ComponentType = decltype((*get_first_of_variadic_template<Type...>)());
                uint32_t typeID = type_id<ComponentType>();
                if (entity == null
                    || index > entities.size()
                    || to_version(entities[index]) != to_version(entity)
                    || pools.size() <= typeID
                    || pools[typeID].handler == nullptr
                    || pools[typeID].entity_set.empty()
                    || pools[typeID].entity_set.find(index) == -1
                    ) {
                    return static_cast<ComponentType*>(nullptr);
                }
                else {
                    int set_index = pools[typeID].entity_set.find(index);
                    auto& handler = (*((pool_handler<ComponentType>*)pools[typeID].handler.get()));
                    return static_cast<ComponentType*>(&handler.get(set_index));
                }
            }
            else {
                return std::forward_as_tuple(try_get<Type>(entity)...);
            }
        }

        template<typename... Type> decltype(auto) get(const entity& entity) const {
            FBY_ASSERT(entity != null, "Failed to get component: Entity is null!");
            const uint32_t index = to_index(entity);
            const uint32_t version = to_version(entities[index]);
            FBY_ASSERT(index < entities.size() && version == to_version(entity), "Failed to get component: Invalid/Outdated handle!");

            if constexpr (sizeof...(Type) == 1) {
                using ComponentType = decltype((*get_first_of_variadic_template<Type...>)());
                uint32_t typeID = type_id<ComponentType>();
                if (pools.size() <= typeID
                    || pools[typeID].handler == nullptr
                    || pools[typeID].entity_set.empty()
                    || pools[typeID].entity_set.find(index) == -1
                    ) {
                    FBY_ERROR("Failed to get component: Component does not exist!");
                    FBY_DEBUGBREAK();
                }
                else {
                    int set_index = pools[typeID].entity_set.find(index);
                    auto& handler = (*((pool_handler<ComponentType>*)pools[typeID].handler.get()));
                    return static_cast<ComponentType&>(handler.get(set_index));
                }
            }
            else {
                return std::forward_as_tuple(get<Type>(entity)...);
            }
        }

        template<typename... Type> bool has(const entity& entity) const {
            FBY_ASSERT(entity != null, "Failed to check component: Entity is null!");
            const uint32_t index = to_index(entity);
            const uint32_t version = to_version(entities[index]);
            FBY_ASSERT(index < entities.size() && version == to_version(entity), "Failed to check component: Invalid/Outdated handle!");

            if constexpr (sizeof...(Type) == 1) {
                using ComponentType = decltype((*get_first_of_variadic_template<Type...>)());
                uint32_t typeID = type_id<ComponentType>();
                if (pools.size() <= typeID
                    || pools[typeID].handler == nullptr
                    || pools[typeID].entity_set.empty()
                    || pools[typeID].entity_set.find(index) == -1
                    ) {
                    return false;
                }
                return true;
            }
            else {
                return (has<Type>(entity) && ...);
            }
        }

        template<typename... Type> void erase(const entity& entity) const {
            FBY_ASSERT(entity != null, "Failed to erase component: Entity is null!");
            const uint32_t index = to_index(entity);
            const uint32_t version = to_version(entities[index]);
            FBY_ASSERT(index < entities.size() && version == to_version(entity), "Failed to erase component: Invalid/Outdated handle!");

            if constexpr (sizeof...(Type) == 1) {
                using ComponentType = decltype((*get_first_of_variadic_template<Type...>)());
                uint32_t typeID = type_id<ComponentType>();
                if (pools.size() <= typeID
                    || pools[typeID].handler == nullptr
                    || pools[typeID].entity_set.empty()
                    || pools[typeID].entity_set.find(index) == -1
                    ) {
                    return;
                }

                pool_data& pool = *((pool_data*)&pools[typeID]);
                if (pool.entity_set.find(index) != -1) {
                    int set_index = pool.entity_set.remove(index);
                    pool.remove(pool, set_index);
                }
            }
            else {
                ((void)erase<Type>(entity), ...);
            }
        }

        void clear() {
            pools.clear();
            entities.clear();
            free_entities.clear();
        }
    private:
        std::vector<pool_data> pools;
        std::vector<entity> entities;
        std::vector<uint32_t> free_entities;
    };
}
