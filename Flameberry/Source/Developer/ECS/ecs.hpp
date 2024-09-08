#pragma once

#include <iostream>
#include <cstdint>
#include <vector>

#include "Core/Core.h"

/**
 * Note (Aditya): This file is converted from snake_case to PascalCase but not there are new naming conventions
 * like names of classes are now starting with a Single Alphabet Prefix
 * This is another experiment with the naming conventions and will be consistent with the rest of the codebase soon :)
 *
 * Naming Conventions :-
 * Class names have certain single letter capital prefixes inspired from Unreal Engine Source Code
 *
 * Prefixes:
 * F -> Flameberry Class
 * T -> Template Class
 * E -> Enum
 * D -> Derived Class
 * I -> Interface
 * A -> Asset Class
 */

namespace Flameberry {

	struct UTypeCounter
	{
		inline static uint32_t TypeCounter = 0;

		template <typename Type>
		friend uint32_t GetStaticTypeID();
	};

	template <typename Type>
	uint32_t GetStaticTypeID()
	{
		static uint32_t componentCounter = UTypeCounter::TypeCounter++;
		return componentCounter;
	}

	template <typename TDataType>
	class TSparseSet
	{
	public:
		void Insert(TDataType value)
		{
			// Check if value exists in set
			if (int32_t index = Find(value); index != -1)
			{
				m_PackedBuffer[index] = value;
				return;
			}
			else if (value >= m_SparseBuffer.size())
				m_SparseBuffer.resize(value + 1);

			m_SparseBuffer[value] = (TDataType)m_PackedBuffer.size();
			m_PackedBuffer.emplace_back(value);
		}

		int32_t Remove(TDataType value)
		{
			if (value >= m_SparseBuffer.size() || m_SparseBuffer[value] >= m_PackedBuffer.size())
			{
				FBY_WARN("Attempted to remove element '{}' from sparse set that didn't belong to set!", value);
				return -1;
			}
			// copying the last element of packed array to the removed element location
			TDataType last = m_PackedBuffer.back();
			m_PackedBuffer[m_SparseBuffer[value]] = last;
			m_SparseBuffer[last] = m_SparseBuffer[value];
			m_PackedBuffer.pop_back();
			return m_SparseBuffer[value];
		}

		/**
		 * Find the location of the value found in packed array
		 *
		 * @param value The value to be searched in the sparse set
		 * @return Location or -1 if the value is not found
		 */
		int32_t Find(TDataType value) const
		{
			if (value >= m_SparseBuffer.size() || m_SparseBuffer[value] >= m_PackedBuffer.size() || m_PackedBuffer[m_SparseBuffer[value]] != value)
				return -1;
			return m_SparseBuffer[value];
		}

		inline uint32_t Size() const { return (uint32_t)m_PackedBuffer.size(); }
		inline bool Empty() const { return m_PackedBuffer.empty(); }

		inline void Clear()
		{
			m_PackedBuffer.clear();
			m_SparseBuffer.clear();
		}

		inline TDataType operator[](size_t index) const
		{
			return m_PackedBuffer[index];
		}

#ifdef FBY_DEBUG
		std::string ToString() const
		{
			std::string str = "(";
			for (const auto& element : m_PackedBuffer)
				str += element << ", ";
			str.pop_back();
			char& back = str.back();
			back = ')';
			return str;
		}
#endif
	private:
		std::vector<TDataType> m_PackedBuffer, m_SparseBuffer;
	};

	struct FNullEntity;
	extern FNullEntity Null;

	/**
	 * Contains a 64 bit integer -> Layout of handle: | 32-bit index | 31-bit version | 1-bit validity |
	 */
	class FEntity
	{
	public:
		using THandleType = std::uint64_t;
		using TVersionType = std::uint32_t;

	public:
		constexpr FEntity()
			: m_Handle(0xFFFFFFFF00000000) {}

		constexpr FEntity(THandleType handle)
			: m_Handle(handle) {}

		constexpr FEntity(uint32_t index, TVersionType version, bool validity)
			: m_Handle((THandleType(index) << 32) | (THandleType(version) << 1) | validity) {}

		constexpr FEntity(const FEntity& entity)
			: m_Handle(entity.m_Handle) {}

		constexpr operator THandleType() const { return m_Handle; }

		constexpr void operator=(FEntity entity) { this->m_Handle = entity.m_Handle; }
		constexpr void operator=(THandleType handle) { this->m_Handle = handle; }
		constexpr void operator=(const FNullEntity& n) { this->m_Handle = 0xFFFFFFFF00000000; }
		constexpr bool operator==(const FEntity& entity) const { return this->m_Handle == entity.m_Handle; }
		constexpr bool operator!=(const FEntity& entity) const { return !(*this == entity); }

		/**
		 * TODO: Add Comments
		 */
		constexpr uint32_t GetIndex() const { return static_cast<uint32_t>(m_Handle >> 32); }

		constexpr TVersionType GetVersion() const { return static_cast<FEntity::TVersionType>(m_Handle) >> 1; }

		constexpr bool GetValidity() const { return m_Handle & 0x1; }

	private:
		THandleType m_Handle;
	};

	/**
	 * Important Class
	 * Depicts a Null FEntity Handle, it's instance acts like a null entity literal
	 */
	struct FNullEntity
	{
		constexpr operator FEntity() const { return 0xFFFFFFFF00000000; }
	};

	template <typename TComponent>
	class TComponentPoolHandler
	{
	public:
		[[nodiscard]] inline TComponent& Get(uint32_t index) { return m_ComponentBuffer[index]; }

		template <typename... Args>
		TComponent& Emplace(Args... args) { return (TComponent&)m_ComponentBuffer.emplace_back(std::forward<Args>(args)...); }

		void Remove(uint32_t index)
		{
			m_ComponentBuffer[index] = m_ComponentBuffer.back();
			m_ComponentBuffer.pop_back();
		}

	private:
		[[nodiscard]] inline std::vector<TComponent>& GetBuffer() { return m_ComponentBuffer; }

	private:
		std::vector<TComponent> m_ComponentBuffer;

		friend class FRegistry;
	};

	struct FComponentPool
	{
		TSparseSet<uint32_t> EntitySet;
		std::shared_ptr<void> ComponentPoolHandler{ nullptr }; // Using std::shared_ptr as using Ref<> and CreateRef<> issues C++20 feature warning

		void (*RemoveFn)(const FComponentPool& pool, uint32_t index);
		void (*CopyHandlerDataFn)(const FComponentPool& src, FComponentPool& dest);

		FComponentPool() = default;
		explicit FComponentPool(const FComponentPool& pool)
			: EntitySet(pool.EntitySet), CopyHandlerDataFn(pool.CopyHandlerDataFn), RemoveFn(pool.RemoveFn)
		{
			if (pool.CopyHandlerDataFn != nullptr)
			{
				// TODO: Check if this needs optimisation, currently the handler data is only copied upon loading the first scene and pressing the play button
				pool.CopyHandlerDataFn(pool, *this);
				FBY_TRACE("Copied ecs component pool handler data!");
			}
		}
	};

	class FRegistry
	{
	public:
		template <typename TComponent>
		class TRegistryView
		{
		public:
			TRegistryView() {}
			TRegistryView(const FRegistry* reg, const FComponentPool* pool)
				: m_RegistryRef(reg), m_ComponentPoolRef(pool) {}

			typename std::vector<TComponent>::iterator begin()
			{
				if (m_RegistryRef && m_ComponentPoolRef && !m_ComponentPoolRef->EntitySet.Empty())
				{
					auto& handler = (*((TComponentPoolHandler<TComponent>*)m_ComponentPoolRef->ComponentPoolHandler.get()));
					return handler.GetBuffer().begin();
				}
				return {};
			}

			typename std::vector<TComponent>::iterator end()
			{
				if (m_RegistryRef && m_ComponentPoolRef && !m_ComponentPoolRef->EntitySet.Empty())
				{
					auto& handler = (*((TComponentPoolHandler<TComponent>*)m_ComponentPoolRef->ComponentPoolHandler.get()));
					return handler.GetBuffer().end();
				}
				return {};
			}

		private:
			const FRegistry* m_RegistryRef = nullptr;
			const FComponentPool* m_ComponentPoolRef = nullptr;
		};

		template <typename... TComponents>
		class TRegistryGroup
		{
		public:
			template <typename... TIteratorType>
			class iterator
			{
			public:
				iterator(const FRegistry* reg, const FComponentPool* pool, uint32_t index)
					: m_RegistryRef(reg), m_ComponentPoolRef(pool), m_Index(index) {}

				iterator<TIteratorType...>& operator++()
				{
					while (++m_Index < m_ComponentPoolRef->EntitySet.Size() && !m_RegistryRef->HasComponent<TIteratorType...>(m_RegistryRef->m_EntityBuffer[m_ComponentPoolRef->EntitySet[m_Index]]))
						;
					return *this;
				}

				iterator<TIteratorType...> operator++(int)
				{
					auto it = *this;
					++(*this);
					return it;
				}

				FEntity operator*() { return m_RegistryRef->m_EntityBuffer[m_ComponentPoolRef->EntitySet[m_Index]]; }

				bool operator==(const iterator<TIteratorType...>& it) { return this->m_Index == it.m_Index; }
				bool operator!=(const iterator<TIteratorType...>& it) { return !(*this == it); }

			private:
				const FRegistry* m_RegistryRef;
				const FComponentPool* m_ComponentPoolRef;
				uint32_t m_Index;
			};

		public:
			TRegistryGroup()
				: m_BeginIndex(0), m_EndIndex(0) {}

			TRegistryGroup(const FRegistry* reg, const FComponentPool* pool)
				: m_RegistryRef(reg), m_ComponentPoolRef(pool), m_BeginIndex(0), m_EndIndex(0)
			{
				while (m_BeginIndex < pool->EntitySet.Size() && !reg->HasComponent<TComponents...>(reg->m_EntityBuffer[pool->EntitySet[m_BeginIndex]]))
					m_BeginIndex++;
				m_EndIndex = pool->EntitySet.Size();
			}

			iterator<TComponents...> begin()
			{
				return iterator<TComponents...>(m_RegistryRef, m_ComponentPoolRef, m_BeginIndex);
			}

			iterator<TComponents...> end()
			{
				return iterator<TComponents...>(m_RegistryRef, m_ComponentPoolRef, m_EndIndex);
			}

		private:
			const FRegistry* m_RegistryRef = nullptr;
			const FComponentPool* m_ComponentPoolRef = nullptr;
			int32_t m_BeginIndex, m_EndIndex;
		};

	public:
		FEntity GetEntityAtIndex(uint32_t index)
		{
			FBY_ASSERT(index < m_EntityBuffer.size() && m_EntityBuffer[index].GetValidity(), "Failed to get entity at index: Invalid index/entity!");
			return m_EntityBuffer[index];
		}

		/**
		 * Returns true if there are no entities in the registry
		 */
		inline bool Empty() const { return m_EntityBuffer.empty(); }

		/**
		 * Iterates over all entities in the scene
		 * @param _Fn: A function with a param of type `const ecs::entity&` which represents the current entity being iterated
		 */
		template <typename Fn>
		void ForEach(Fn&& _Fn)
		{
			static_assert(std::is_invocable_v<Fn, FEntity>);
			for (auto entity : m_EntityBuffer)
			{
				if (entity.GetValidity())
					_Fn(entity);
			}
		}

		/**
		 * Iterates over all components in the scene
		 *
		 * @param _Fn: A function with a param of type `void*` which represents the current component being iterated and `uint32_t` which gives the typeID of the component
		 */
		template <typename Fn>
		void ForEachComponent(FEntity entity, Fn&& _Fn)
		{
			FBY_ASSERT(0, "for_each_component() Not Yet Implemented!");

			static_assert(std::is_invocable_v<Fn, void*, uint32_t>);

			FBY_ASSERT(entity != Null, "Failed to iterate over components of entity: Entity is null!");

			const uint32_t index = entity.GetIndex();
			const uint32_t version = m_EntityBuffer[index].GetVersion();

			FBY_ASSERT(index < m_EntityBuffer.size() && version == entity.GetVersion(), "Failed to iterate over components of entity: Invalid handle!");

			uint32_t typeID = 0;
			for (auto& pool : m_ComponentPools)
			{
				if (int32_t setIndex = pool.EntitySet.Find(index); setIndex != -1)
				{
					// auto& handler = (*((pool_handler<Type>*)pool.handler.get()));
					// _Fn(&handler.get(setIndex), typeID);
				}
				typeID++;
			}
		}

		/**
		 * Returns a registry view for the specified type.
		 *
		 * @tparam Type The type of components to retrieve from the registry.
		 * @return A registry view for the specified type.
		 */
		template <typename TComponent>
		TRegistryView<TComponent> View()
		{
			uint32_t typeID = GetStaticTypeID<TComponent>();

			if (typeID >= m_ComponentPools.size())
				return TRegistryView<TComponent>();
			return TRegistryView<TComponent>(this, &m_ComponentPools[typeID]);
		}

		/**
		 * Creates a registry group containing entities that have components of the specified types.
		 *
		 * @tparam Type The component types to filter entities by.
		 * @return A registry group containing entities with the specified component types.
		 */
		template <typename... TComponents>
		TRegistryGroup<TComponents...> Group()
		{
			static_assert(sizeof...(TComponents) > 0);

			uint32_t typeIDs[] = { GetStaticTypeID<TComponents>()... };
			uint32_t smallestPoolIndex = 0;
			uint32_t smallestPoolSize = UINT32_MAX;

			for (const auto& typeID : typeIDs)
			{
				if (typeID >= m_ComponentPools.size())
				{
					return TRegistryGroup<TComponents...>();
				}
				if (uint32_t poolSize = m_ComponentPools[typeID].EntitySet.Size(); poolSize < smallestPoolSize)
				{
					smallestPoolSize = poolSize;
					smallestPoolIndex = typeID;
				}
			}
			return TRegistryGroup<TComponents...>(this, &m_ComponentPools[smallestPoolIndex]);
		}

		FEntity CreateEntity()
		{
			if (!m_FreeEntityBuffer.empty())
			{
				const uint32_t freeEntityIndex = m_FreeEntityBuffer.back();
				const uint32_t version = m_EntityBuffer[freeEntityIndex].GetVersion();
				m_EntityBuffer[freeEntityIndex] = FEntity(freeEntityIndex, version + 1, true);

				m_FreeEntityBuffer.pop_back();
				return m_EntityBuffer[freeEntityIndex];
			}
			return m_EntityBuffer.emplace_back(FEntity(static_cast<uint32_t>(m_EntityBuffer.size()), 0, true));
		}

		void DestroyEntity(FEntity entity)
		{
			FBY_ASSERT(entity != Null, "Failed to destroy entity: Entity is null!");

			const uint32_t index = entity.GetIndex();
			const uint32_t version = m_EntityBuffer[index].GetVersion();

			FBY_ASSERT(index < m_EntityBuffer.size() && version == entity.GetVersion(), "Failed to delete entity: Invalid handle!");

			for (auto& pool : m_ComponentPools)
			{
				if (pool.EntitySet.Find(index) != -1)
				{
					int32_t setIndex = pool.EntitySet.Remove(index);
					pool.RemoveFn(pool, setIndex);
				}
			}
			m_FreeEntityBuffer.emplace_back(index);
			m_EntityBuffer[index] = m_EntityBuffer[index] & 0xFFFFFFFFFFFFFFFE;
		}

		template <typename TComponent, typename... TArgs>
		TComponent& EmplaceComponent(const FEntity& entity, TArgs... args)
		{
			FBY_ASSERT(entity != Null, "Failed to emplace component: Entity is null!");
			const uint32_t index = entity.GetIndex();
			const uint32_t version = m_EntityBuffer[index].GetVersion();
			FBY_ASSERT(index < m_EntityBuffer.size() && version == entity.GetVersion(), "Failed to emplace component: Invalid/Outdated handle!");

			const uint32_t typeID = GetStaticTypeID<TComponent>();
			if (m_ComponentPools.size() <= typeID)
			{
				m_ComponentPools.resize(typeID + 1);
				auto& pool = m_ComponentPools.back();
				pool.ComponentPoolHandler = std::make_shared<TComponentPoolHandler<TComponent>>();
			}
			else if (m_ComponentPools[typeID].ComponentPoolHandler == nullptr)
				m_ComponentPools[typeID].ComponentPoolHandler = std::make_shared<TComponentPoolHandler<TComponent>>();
			else if (m_ComponentPools[typeID].EntitySet.Find(index) != -1)
				FBY_ASSERT(0, "Failed to emplace component: Entity already has component!");

			m_ComponentPools[typeID].EntitySet.Insert(index);

			if (m_ComponentPools[typeID].RemoveFn == nullptr)
			{
				m_ComponentPools[typeID].RemoveFn = [](const FComponentPool& pool, uint32_t index)
				{
					auto& handler = (*((TComponentPoolHandler<TComponent>*)pool.ComponentPoolHandler.get()));
					handler.Remove(index);
				};
			}

			if (m_ComponentPools[typeID].CopyHandlerDataFn == nullptr)
			{
				m_ComponentPools[typeID].CopyHandlerDataFn = [](const FComponentPool& src, FComponentPool& dest)
				{
					if (src.ComponentPoolHandler)
					{
						auto& handler = (*((TComponentPoolHandler<TComponent>*)src.ComponentPoolHandler.get()));
						dest.ComponentPoolHandler = std::make_shared<TComponentPoolHandler<TComponent>>(handler);
					}
				};
			}

			auto& handler = (*((TComponentPoolHandler<TComponent>*)m_ComponentPools[typeID].ComponentPoolHandler.get()));
			return handler.Emplace(std::forward<TArgs>(args)...);
		}

		template <typename TComponent, typename... TMoreComponents>
		decltype(auto) TryGetComponent(const FEntity& entity) const
		{
			if constexpr (sizeof...(TMoreComponents) == 0)
			{
				const uint32_t index = entity.GetIndex();

				uint32_t typeID = GetStaticTypeID<TComponent>();
				if (entity == Null
					|| index > m_EntityBuffer.size()
					|| m_EntityBuffer[index].GetVersion() != entity.GetVersion()
					|| m_ComponentPools.size() <= typeID
					|| m_ComponentPools[typeID].ComponentPoolHandler == nullptr
					|| m_ComponentPools[typeID].EntitySet.Empty()
					|| m_ComponentPools[typeID].EntitySet.Find(index) == -1)
				{
					return static_cast<TComponent*>(nullptr);
				}
				else
				{
					int32_t setIndex = m_ComponentPools[typeID].EntitySet.Find(index);
					auto& handler = (*((TComponentPoolHandler<TComponent>*)m_ComponentPools[typeID].ComponentPoolHandler.get()));
					return static_cast<TComponent*>(&handler.Get(setIndex));
				}
			}
			else
			{
				return std::forward_as_tuple(TryGetComponent<TComponent>(entity), TryGetComponent<TMoreComponents>(entity)...);
			}
		}

		template <typename TComponent, typename... TMoreComponents>
		decltype(auto) GetComponent(const FEntity& entity) const
		{
			FBY_ASSERT(entity != Null, "Failed to get component: Entity is null!");
			const uint32_t index = entity.GetIndex();
			const uint32_t version = m_EntityBuffer[index].GetVersion();
			FBY_ASSERT(index < m_EntityBuffer.size() && version == entity.GetVersion(), "Failed to get component: Invalid/Outdated handle!");

			if constexpr (sizeof...(TMoreComponents) == 0)
			{
				uint32_t typeID = GetStaticTypeID<TComponent>();
				if (m_ComponentPools.size() <= typeID
					|| m_ComponentPools[typeID].ComponentPoolHandler == nullptr
					|| m_ComponentPools[typeID].EntitySet.Empty()
					|| m_ComponentPools[typeID].EntitySet.Find(index) == -1)
				{
					FBY_ERROR("Failed to get component: Component does not exist!");
					FBY_DEBUGBREAK();
				}
				else
				{
					int32_t setIndex = m_ComponentPools[typeID].EntitySet.Find(index);
					auto& handler = (*((TComponentPoolHandler<TComponent>*)m_ComponentPools[typeID].ComponentPoolHandler.get()));
					return static_cast<TComponent&>(handler.Get(setIndex));
				}
			}
			else
			{
				return std::forward_as_tuple(GetComponent<TComponent>(entity), GetComponent<TMoreComponents>(entity)...);
			}
		}

		template <typename TComponent, typename... TMoreComponents>
		bool HasComponent(const FEntity& entity) const
		{
			FBY_ASSERT(entity != Null, "Failed to check component: Entity is null!");
			const uint32_t index = entity.GetIndex();
			const uint32_t version = m_EntityBuffer[index].GetVersion();
			FBY_ASSERT(index < m_EntityBuffer.size() && version == entity.GetVersion(), "Failed to check component: Invalid/Outdated handle!");

			if constexpr (sizeof...(TMoreComponents) == 0)
			{
				uint32_t typeID = GetStaticTypeID<TComponent>();
				if (m_ComponentPools.size() <= typeID
					|| m_ComponentPools[typeID].ComponentPoolHandler == nullptr
					|| m_ComponentPools[typeID].EntitySet.Empty()
					|| m_ComponentPools[typeID].EntitySet.Find(index) == -1)
				{
					return false;
				}
				return true;
			}
			else
			{
				return (HasComponent<TComponent>(entity) && (HasComponent<TMoreComponents>(entity) && ...));
			}
		}

		template <typename TComponent, typename... TMoreComponents>
		void EraseComponent(const FEntity& entity) const
		{
			FBY_ASSERT(entity != Null, "Failed to erase component: Entity is null!");
			const uint32_t index = entity.GetIndex();
			const uint32_t version = m_EntityBuffer[index].GetVersion();
			FBY_ASSERT(index < m_EntityBuffer.size() && version == entity.GetVersion(), "Failed to erase component: Invalid/Outdated handle!");

			if constexpr (sizeof...(TMoreComponents) == 0)
			{
				uint32_t typeID = GetStaticTypeID<TComponent>();
				if (m_ComponentPools.size() <= typeID
					|| m_ComponentPools[typeID].ComponentPoolHandler == nullptr
					|| m_ComponentPools[typeID].EntitySet.Empty()
					|| m_ComponentPools[typeID].EntitySet.Find(index) == -1)
				{
					return;
				}

				FComponentPool& pool = *((FComponentPool*)&m_ComponentPools[typeID]);
				if (pool.EntitySet.Find(index) != -1)
				{
					int32_t setIndex = pool.EntitySet.Remove(index);
					pool.RemoveFn(pool, setIndex);
				}
			}
			else
			{
				EraseComponent<TComponent>(entity);
				((void)EraseComponent<TMoreComponents>(entity), ...);
			}
		}

		void Clear()
		{
			m_ComponentPools.clear();
			m_EntityBuffer.clear();
			m_FreeEntityBuffer.clear();
		}

	private:
		std::vector<FComponentPool> m_ComponentPools;
		std::vector<FEntity> m_EntityBuffer;
		std::vector<uint32_t> m_FreeEntityBuffer;
	};

} // namespace Flameberry
