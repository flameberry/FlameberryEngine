#pragma once

#include <unordered_map>
#include <set>

#include <vulkan/vulkan.h>

// TODO: Should this include be in the cpp file only with forward declaration and heap allocation of spv_reflect::ShaderModule?
#include <SPIRV-Reflect/spirv_reflect.h>

namespace Flameberry {

	struct ReflectionPushConstantSpecification
	{
		const char* Name;
		VkShaderStageFlagBits VulkanShaderStage;
		uint32_t Size, Offset;
		bool RendererOnly = true;
	};

	struct ReflectionUniformVariableSpecification
	{
		// This `Name` variable currently has no use
		const char* Name;
		uint32_t LocalOffset, GlobalOffset, Size;
	};

	struct ReflectionDescriptorBindingSpecification
	{
		std::string Name; // For Debug Purposes only, May remove in the future
		uint32_t Set, Binding, Count;
		VkDescriptorType Type;
		VkShaderStageFlags VulkanShaderStage;
		bool RendererOnly = true, IsDescriptorTypeImage = false;
	};

	struct ReflectionDescriptorSetSpecification
	{
		uint32_t Set = 0, BindingCount = 0;
	};

	enum class ShaderDataType : uint8_t
	{
		None = 0,
		UInt,
		UInt2,
		UInt3,
		UInt4,
		Int,
		Int2,
		Int3,
		Int4,
		Float,
		Float2,
		Float3,
		Float4,
		Float3x3,
		Float4x4,
		Bool,

		// Ensure that these Dummy Data Types are always at the end of the enum...
		// ...To avoid unwanted effects during creating Vertex Attribute Descriptions in Pipeline
		Dummy1,
		Dummy4,
		Dummy8,
		Dummy12,
		Dummy16 // Note: The sizes are in bytes
	};

	class Shader
	{
	public:
		Shader(const char* vertexShaderSpvPath, const char* fragmentShaderSpvPath);
		Shader(const char* spvBinaryPath);
		~Shader();

		void GetVertexAndFragmentShaderModules(VkShaderModule* outVertexShaderModule, VkShaderModule* outFragmentShaderModule) const;

		const std::string& GetName() const { return m_Name; }
		VkShaderModule GetVulkanShaderModule() const;
		const std::vector<ShaderDataType>& GetInputVariableDataTypes() const { return m_InputDataTypes; }
		const std::set<uint32_t>& GetSpecializationConstantIDSet() const { return m_SpecializationConstantIDSet; }
		const std::vector<ReflectionPushConstantSpecification>& GetPushConstantSpecifications() const { return m_PushConstantSpecifications; }
		const std::vector<ReflectionDescriptorBindingSpecification>& GetDescriptorBindingSpecifications() const { return m_DescriptorBindingSpecifications; }
		const std::vector<ReflectionDescriptorSetSpecification>& GetDescriptorSetSpecifications() const { return m_DescriptorSetSpecifications; }

		// Costly functions
		const ReflectionUniformVariableSpecification& GetUniform(const std::string& name) const;
		const ReflectionDescriptorBindingSpecification& GetBinding(const std::string& name) const;

	private:
		std::vector<char> LoadShaderSpvCode(const char* path);
		void Reflect(const std::vector<char>& shaderSpvBinaryCode);
		VkShaderModule CreateVulkanShaderModule(const std::vector<char>& shaderSpvBinaryCode);

	private:
		std::string m_Name;

		union
		{
			VkShaderModule m_ShaderModule;
			struct
			{
				VkShaderModule m_VertexShaderModule;
				VkShaderModule m_FragmentShaderModule;
			};
		};

		VkShaderStageFlags m_VulkanShaderStageFlags = 0;

		std::set<uint32_t> m_SpecializationConstantIDSet;

		std::vector<ShaderDataType> m_InputDataTypes;
		std::vector<ReflectionPushConstantSpecification> m_PushConstantSpecifications;
		std::vector<ReflectionDescriptorBindingSpecification> m_DescriptorBindingSpecifications;
		std::vector<ReflectionDescriptorSetSpecification> m_DescriptorSetSpecifications;

		// This should only be accessed while initialisation of material or while editing it.
		// Don't access this during rendering every frame. As std::string hashing causes overhead
		// Note: The full names of the uniforms are used to access it, for e.g.: `u_Material.Roughness` instead of just `Roughness`
		// If no alias is given to the push constant block, then only the variable name is considered as full name, e.g.: `u_Roughness`
		std::unordered_map<std::string, ReflectionUniformVariableSpecification> m_UniformFullNameToSpecification;

		// TODO: Should we just use unions to store both Uniform Variables and Descriptor Sets in a generic struct
		// And then have just a single unordered_map to point from the name to the UniformVariable/DescriptorBinding Specification?
		std::unordered_map<std::string, uint32_t> m_DescriptorBindingVariableFullNameToSpecificationIndex;
	};

} // namespace Flameberry
