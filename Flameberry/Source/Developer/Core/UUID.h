#pragma once

#include <iostream>
#include <unordered_map>

namespace Flameberry {
	class UUID
	{
	public:
		using value_type = uint64_t;

	public:
		explicit UUID();
		UUID(value_type uuid);
		~UUID();

		operator value_type() const { return m_UUID; }

	private:
		value_type m_UUID;
	};
} // namespace Flameberry

namespace std {
	template <>
	struct hash<Flameberry::UUID>
	{
		size_t operator()(const Flameberry::UUID& key) const
		{
			return hash<Flameberry::UUID::value_type>()(
				(Flameberry::UUID::value_type)key);
		}
	};
} // namespace std