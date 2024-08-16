#pragma once

#include <filesystem>

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

#include "Core/UUID.h"

namespace YAML {

	template <>
	struct convert<std::filesystem::path>
	{
		static Node encode(const std::filesystem::path& rhs)
		{
			return Node(rhs.string());
		}

		static bool decode(const Node& node, std::filesystem::path& rhs)
		{
			rhs = std::filesystem::path(node.as<std::string>());
			return true;
		}
	};

	template <>
	struct convert<Flameberry::UUID>
	{
		static Node encode(Flameberry::UUID rhs)
		{
			return Node((Flameberry::UUID::ValueType)rhs);
		}

		static bool decode(const Node& node, Flameberry::UUID& rhs)
		{
			rhs = node.as<Flameberry::UUID::ValueType>();
			return true;
		}
	};

	template <>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template <>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

} // namespace YAML

namespace Flameberry {

	YAML::Emitter& operator<<(YAML::Emitter& out, const std::filesystem::path& v);
	YAML::Emitter& operator<<(YAML::Emitter& out, UUID v);
	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v);
	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v);

} // namespace Flameberry
