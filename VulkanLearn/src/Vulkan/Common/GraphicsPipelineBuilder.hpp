#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "../Types.hpp"
#include "../Init.hpp"

namespace Vulkan::Common 
{
	class GraphicsPipelineBuilder
	{
	public:
        virtual ~GraphicsPipelineBuilder() = default;
        inline GraphicsPipelineBuilder(VkPipelineLayout pipelineLayout)
		{
			Clear();
			_pipelineLayout = pipelineLayout;
        }

		inline VkPipeline Build(VkDevice device)
		{
            VkPipelineViewportStateCreateInfo viewportState
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .pNext = nullptr,
                .viewportCount = 1,
                .scissorCount = 1,
            };

            VkPipelineColorBlendStateCreateInfo colorBlending
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .pNext = nullptr,
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_COPY,
                .attachmentCount = 1,
                .pAttachments = &_colorBlendAttachment,
            };

            VkPipelineVertexInputStateCreateInfo _vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

            VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            VkPipelineDynamicStateCreateInfo dynamicInfo
            { 
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = 2,
                .pDynamicStates = &state[0],
            };

            VkGraphicsPipelineCreateInfo pipelineInfo
            { 
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &_renderInfo,
                .stageCount = (uint32_t)_shaderStages.size(),
                .pStages = _shaderStages.data(),
                .pVertexInputState = &_vertexInputInfo,
                .pInputAssemblyState = &_inputAssembly,
                .pViewportState = &viewportState,
                .pRasterizationState = &_rasterizer,
                .pMultisampleState = &_multisampling,
                .pDepthStencilState = &_depthStencil,
                .pColorBlendState = &colorBlending,
				.pDynamicState = &dynamicInfo,
                .layout = _pipelineLayout,
			};

            VkPipeline pipeline;
			VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline))

			return pipeline;
		}

        inline GraphicsPipelineBuilder& SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader)
        {
            _shaderStages.clear();

			auto vertexStage = Vulkan::Init::pipelineShaderStageCreateInfo(vertexShader, VK_SHADER_STAGE_VERTEX_BIT);
			_shaderStages.push_back(vertexStage);

			auto fragmentStage = Vulkan::Init::pipelineShaderStageCreateInfo(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT);
            _shaderStages.push_back(fragmentStage);

            return *this;
        }

        inline GraphicsPipelineBuilder& SetInputTopology(VkPrimitiveTopology topology)
        {
            _inputAssembly.topology = topology;
            _inputAssembly.primitiveRestartEnable = VK_FALSE;

            return *this;
        }

        inline GraphicsPipelineBuilder& SetPolygonMode(VkPolygonMode mode)
        {
            _rasterizer.polygonMode = mode;
            _rasterizer.lineWidth = 1.f;

            return *this;
        }

        inline GraphicsPipelineBuilder& SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
        {
            _rasterizer.cullMode = cullMode;
            _rasterizer.frontFace = frontFace;

            return *this;
        }

        inline GraphicsPipelineBuilder& SetMultisamplingNone()
        {
            _multisampling.sampleShadingEnable = VK_FALSE;
            _multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            _multisampling.minSampleShading = 1.0f;
            _multisampling.pSampleMask = nullptr;
            _multisampling.alphaToCoverageEnable = VK_FALSE;
            _multisampling.alphaToOneEnable = VK_FALSE;

            return *this;
        }

        inline GraphicsPipelineBuilder& DisableBlending()
        {
            _colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT 
                | VK_COLOR_COMPONENT_G_BIT 
                | VK_COLOR_COMPONENT_B_BIT 
                | VK_COLOR_COMPONENT_A_BIT;
            _colorBlendAttachment.blendEnable = VK_FALSE;

            return *this;
        }

        inline GraphicsPipelineBuilder& SetColorAttachmentFormat(VkFormat format)
        {
            _colorAttachmentformat = format;
            _renderInfo.colorAttachmentCount = 1;
            _renderInfo.pColorAttachmentFormats = &_colorAttachmentformat;

            return *this;
        }

        inline GraphicsPipelineBuilder& SetDepthFormat(VkFormat format)
        {
            _renderInfo.depthAttachmentFormat = format;

            return *this;
        }

        inline GraphicsPipelineBuilder& DisableDepthTesting()
        {
            _depthStencil.depthTestEnable = VK_FALSE;
            _depthStencil.depthWriteEnable = VK_FALSE;
            _depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
            _depthStencil.depthBoundsTestEnable = VK_FALSE;
            _depthStencil.stencilTestEnable = VK_FALSE;
            _depthStencil.front = {};
            _depthStencil.back = {};
            _depthStencil.minDepthBounds = 0.f;
            _depthStencil.maxDepthBounds = 1.f;

            return *this;
        }

        inline GraphicsPipelineBuilder& EnableDepthtest(bool depthWriteEnable, VkCompareOp op)
        {
            _depthStencil.depthTestEnable = VK_TRUE;
            _depthStencil.depthWriteEnable = depthWriteEnable;
            _depthStencil.depthCompareOp = op;
            _depthStencil.depthBoundsTestEnable = VK_FALSE;
            _depthStencil.stencilTestEnable = VK_FALSE;
            _depthStencil.front = {};
            _depthStencil.back = {};
            _depthStencil.minDepthBounds = 0.f;
            _depthStencil.maxDepthBounds = 1.f;

            return *this;
        }

        inline GraphicsPipelineBuilder& enablBlendingAditive()
        {
            _colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            _colorBlendAttachment.blendEnable = VK_TRUE;
            _colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            _colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            _colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            _colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            _colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            _colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

            return *this;
        }

        inline GraphicsPipelineBuilder& EnableBlendingAlphablend()
        {
            _colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            _colorBlendAttachment.blendEnable = VK_TRUE;
            _colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            _colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            _colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            _colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            _colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            _colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

            return *this;
        }

        inline GraphicsPipelineBuilder& SetPipelineLayout(VkPipelineLayout layout)
		{
			_pipelineLayout = layout;

            return *this;
		}

        inline void Clear()
		{
			_pipelineLayout = VK_NULL_HANDLE;
            _inputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
            _rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
            _colorBlendAttachment = {};
            _multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
            _pipelineLayout = {};
            _depthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
            _renderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
			_shaderStages.clear();
		}

	private:
        std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

        VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
        VkPipelineRasterizationStateCreateInfo _rasterizer;
        VkPipelineColorBlendAttachmentState _colorBlendAttachment;
        VkPipelineMultisampleStateCreateInfo _multisampling;
        VkPipelineLayout _pipelineLayout;
        VkPipelineDepthStencilStateCreateInfo _depthStencil;
        VkPipelineRenderingCreateInfo _renderInfo;
        VkFormat _colorAttachmentformat;
	};
}