#pragma once

#include <type_traits>

#include "Core/Core.h"

#include "SwapChain.h"

namespace Flameberry {

	template <typename TResource, uint32_t N = SwapChain::MAX_FRAMES_IN_FLIGHT>
	class TFrameResource
	{
	public:
		~TFrameResource()
		{
			for (auto& replica : m_Replicas)
				replica = nullptr;
		}

		inline Ref<TResource>& operator[](int index)
		{
			return m_Replicas[index];
		}

		inline const Ref<TResource>& operator[](int index) const
		{
			return m_Replicas[index];
		}

		template <typename Fn>
			requires std::is_invocable_v<Fn, Ref<TResource>&, uint32_t>
		inline void ForEach(Fn&& func)
		{
			for (int i = 0; i < N; i++)
				func(m_Replicas[i], i);
		}

	private:
		Ref<TResource> m_Replicas[N];
	};

} // namespace Flameberry
