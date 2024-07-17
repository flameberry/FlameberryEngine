#pragma once

#include <iostream>
#include <unordered_map>

namespace Flameberry {

	class UUID
	{
	public:
		using ValueType = uint64_t;

	public:
		explicit UUID();
		UUID(ValueType uuid);
		~UUID();

		operator ValueType() const { return m_UUID; }

	private:
		ValueType m_UUID;
	};

} // namespace Flameberry

namespace std {

	template <>
	struct hash<Flameberry::UUID>
	{
		size_t operator()(const Flameberry::UUID& key) const
		{
			return hash<Flameberry::UUID::ValueType>()((Flameberry::UUID::ValueType)key);
		}
	};

} // namespace std