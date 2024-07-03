#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "RenderPass.h"
#include "DescriptorSet.h"
#include "Shader.h"

namespace Flameberry {

	using VertexInputLayout = std::vector<ShaderDataType>;

	struct PipelineSpecification
	{
		Ref<RenderPass> RenderPass;
		Ref<Shader>		Shader;

		// Make sure to provide all the attributes present in the Vertex Buffer
		// And for those that don't need to be used in the shader replace them with the equivalent Dummy Types
		VertexInputLayout VertexLayout;

		uint32_t			SubPass = 0;
		uint32_t			Samples = 1;
		VkViewport			Viewport = { 0, 0, 0, 0, 0.0f, 1.0f };
		VkRect2D			Scissor;
		bool				BlendingEnable = false, DepthTestEnable = true, DepthWriteEnable = true, DepthClampEnable = false, StencilTestEnable = false;
		VkCompareOp			DepthCompareOp = VK_COMPARE_OP_LESS;
		bool				DynamicStencilEnable = false, DynamicStencilOp = false;
		VkStencilOpState	StencilOpState = {};
		VkCullModeFlags		CullMode = VK_CULL_MODE_BACK_BIT;
		VkFrontFace			FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		VkPolygonMode		PolygonMode = VK_POLYGON_MODE_FILL;
		VkPrimitiveTopology PrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	};

	class Pipeline
	{
	public:
		Pipeline(const PipelineSpecification& pipelineSpec);
		~Pipeline();

		const PipelineSpecification& GetSpecification() const { return m_Specification; }
		VkPipelineLayout			 GetVulkanPipelineLayout() const { return m_PipelineLayout; }
		VkPipeline					 GetVulkanPipeline() const { return m_GraphicsPipeline; }

		void ReloadShaders();

	private:
		void CreatePipeline();

	private:
		PipelineSpecification m_Specification;
		VkPipeline			  m_GraphicsPipeline;
		VkPipelineLayout	  m_PipelineLayout;

		std::vector<Ref<DescriptorSetLayout>> m_DescriptorSetLayouts;
	};

	struct ComputePipelineSpecification
	{
		Ref<Shader> Shader;
	};

	class ComputePipeline
	{
	public:
		ComputePipeline(const ComputePipelineSpecification& pipelineSpec);
		~ComputePipeline();

		Ref<DescriptorSetLayout> GetDescriptorSetLayout(uint32_t setIndex) { return m_DescriptorSetLayouts[setIndex]; }

		ComputePipelineSpecification GetSpecification() const { return m_Specification; }
		VkPipelineLayout			 GetVulkanPipelineLayout() const { return m_PipelineLayout; }
		VkPipeline					 GetVulkanPipeline() const { return m_ComputePipeline; }

	private:
		ComputePipelineSpecification m_Specification;
		VkPipeline					 m_ComputePipeline;
		VkPipelineLayout			 m_PipelineLayout;

		std::vector<Ref<DescriptorSetLayout>> m_DescriptorSetLayouts;
	};

} // namespace Flameberry
