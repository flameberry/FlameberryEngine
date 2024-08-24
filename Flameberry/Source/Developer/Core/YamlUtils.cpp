#include "YamlUtils.h"

namespace Flameberry {
    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
        return out;
    }
    

	YAML::Emitter& operator<<(YAML::Emitter& out, const std::filesystem::path& v)
	{
		out << v.string();
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, UUID v)
	{
		out.WriteIntegralType((UUID::ValueType)v);
        return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

} // namespace Flameberry
