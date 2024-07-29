#include "Renderer/Renderer.h"

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	const char* type = "Unknown";
	switch (messageType)
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
		type = "GENERAL";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
		type = "VALIDACION";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
		type = "RENDIMIENTO";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
		type = "DEVICE ADDRESS BINDING";
		break;
	}

	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		spdlog::debug("[VULKAN] Tipo: {0}, Mensaje: {1}", type, pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		spdlog::info("[VULKAN] Tipo: {0}, Mensaje: {1}", type, pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		spdlog::warn("[VULKAN] Tipo: {0}, Mensaje: {1}", type, pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		spdlog::error("[VULKAN] Tipo: {0}, Mensaje: {1}", type, pCallbackData->pMessage);
		break;
	}

	return VK_FALSE;
}

namespace vl::core
{
    Renderer::Renderer(GLFWwindow* pWindow, uint32_t width, uint32_t height) :
        _pWindow(pWindow),
        _width(width),
        _height(height)
    {
		
    }

    std::unique_ptr<Renderer> Renderer::Create(GLFWwindow* pWindow, uint32_t width, uint32_t height)
    {
        return std::make_unique<Renderer>(pWindow, width, height);
    }

    void Renderer::OnResize(uint32_t width, uint32_t height)
    {
		_width = width;
		_height = height;
		_resized = true;
    }

	void Renderer::OnImGuiRender(ImGuiRenderFn imguiRenderFuntion)
	{
		if (_resized) {
			RecreateSwapchain();
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		static bool open = true;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		ImGuiWindowFlags window_flags = /*ImGuiWindowFlags_MenuBar |*/ ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("DockSpace Demo", &open, window_flags);

		ImGui::PopStyleVar();

		ImGui::PopStyleVar(2);

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		imguiRenderFuntion();

		ImGui::End();

		ImGui::Render();
	}

    void Renderer::Init()
	{
		InitVulkan();
		InitSyncObjects();
		InitSwapchain();
		InitCommands();
		InitGraphicsPipeline();
		InitImGui();
    }

    void Renderer::OnRender()
    {
		vulkan::Frame& currentFrame = CurrentFrame();

		VK_CHECK(vkWaitForFences(_logicalDevice, 1, &currentFrame.Fence, VK_TRUE, UINT64_MAX));

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(_logicalDevice, _swapchain.Handle, UINT64_MAX, currentFrame.ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			_resized = true;
			RecreateSwapchain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::exception("No fue posible obtener la imagen");
		}

		VK_CHECK(vkResetFences(_logicalDevice, 1, &currentFrame.Fence));

		_drawData.ImageExtent.width = glm::min(_swapchain.Extent.width, _drawData.Image.Extent.width);
		_drawData.ImageExtent.height = glm::min(_swapchain.Extent.height, _drawData.Image.Extent.height);

		VkCommandBuffer currentCommandBuffer = currentFrame.CommandBuffer;

		VK_CHECK(vkResetCommandBuffer(currentCommandBuffer, 0));

		RecordCommandBuffer(currentCommandBuffer, imageIndex);

		VkCommandBufferSubmitInfo commandBufferInfo = vulkan::commandBufferSubmitInfo(currentCommandBuffer);

		VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo = vulkan::semaphoreSubmitInfo(
			currentFrame.ImageAvailableSemaphore,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR);

		VkSemaphoreSubmitInfo signalSemaphoreSubmitInfo = vulkan::semaphoreSubmitInfo(
			currentFrame.RenderFinishedSemaphore,
			VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

		VkSubmitInfo2 submitInfo2 = vulkan::submitInfo2(commandBufferInfo, &waitSemaphoreSubmitInfo, &signalSemaphoreSubmitInfo);

		VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submitInfo2, currentFrame.Fence));

		VkPresentInfoKHR presentInfo = vulkan::presentInfoKHR(&_swapchain.Handle, &imageIndex, &currentFrame.RenderFinishedSemaphore);

		result = vkQueuePresentKHR(_presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _resized) {
			_resized = true;
			RecreateSwapchain();
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::exception("No fue posible presentar en la cola de presentacion");
		}

		_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

	const GraphicsPipeline& Renderer::GenerateGenericGraphicsPipeline(
		const std::string& name, 
		const std::shared_ptr<Shader>& vertexShader, 
		const std::shared_ptr<Shader>& fragmentShader)
	{
		spdlog::info("Building Generic Graphics Pipeline: {}", name);

		if (_graphicsPipelines.count(name) > 0)
		{
			spdlog::error("Ya existe un pipeline con ese nombre: {0}", name);
			throw std::exception("Ya existe un pipeline con ese nombre");
		}

		VkShaderModule vertexShaderModule = vertexShader->CreateShaderModule(_logicalDevice);
		VkShaderModule fragmentShaderModule = fragmentShader->CreateShaderModule(_logicalDevice);

		GraphicsPipeline pipeline
		{
			.Handle = BuildGenericGraphicsPipeline(vertexShaderModule, fragmentShaderModule)
		};

		_graphicsPipelines[name] = pipeline;

		vkDestroyShaderModule(_logicalDevice, vertexShaderModule, nullptr);
		vkDestroyShaderModule(_logicalDevice, fragmentShaderModule, nullptr);

		return _graphicsPipelines[name];
	}

	const void Renderer::BindGraphicsPipeline(VkCommandBuffer commandBuffer, const GraphicsPipeline& pipeline) const
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.Handle);

		VkViewport viewport
		{
			.x = 0,
			.y = 0,
			.width = float(_drawData.ImageExtent.width),
			.height = float(_drawData.ImageExtent.height),
			.minDepth = 0.f,
			.maxDepth = 1.f,
		};

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor
		{
			.offset
			{
				.x = 0,
				.y = 0
			},
			.extent
			{
				.width = _drawData.ImageExtent.width,
				.height = _drawData.ImageExtent.height,
			}
		};

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void Renderer::PushBackgroundRenderFunction(RenderFn renderFunction)
	{
		_backgroundRenderFunctions.push_back(renderFunction);
	}

	void Renderer::PushRenderFunction(RenderFn renderFunction)
	{
		_renderFunctions.push_back(renderFunction);
	}

    void Renderer::Shutdown()
    {
		VK_CHECK(vkDeviceWaitIdle(_logicalDevice));

		DestroyImgui();
		
		for (auto& [name, pipeline] : _graphicsPipelines)
		{
			vkDestroyPipeline(_logicalDevice, pipeline.Handle, nullptr);
		}

		vkDestroyPipelineLayout(_logicalDevice, _pipelineLayout, nullptr);

		vkDestroyImageView(_logicalDevice, _drawData.DepthImage.View, nullptr);
		vmaDestroyImage(_allocator, _drawData.DepthImage.Handle, _drawData.DepthImage.Allocation);

		vkDestroyImageView(_logicalDevice, _drawData.Image.View, nullptr);
		vmaDestroyImage(_allocator, _drawData.Image.Handle, _drawData.Image.Allocation);

		vkDestroyCommandPool(_logicalDevice, _immediateData.CommandPool, nullptr);
		vkDestroyFence(_logicalDevice, _immediateData.Fence, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(_logicalDevice, _frames[i].ImageAvailableSemaphore, nullptr);
			vkDestroySemaphore(_logicalDevice, _frames[i].RenderFinishedSemaphore, nullptr);
			vkDestroyFence(_logicalDevice, _frames[i].Fence, nullptr);
			vkDestroyCommandPool(_logicalDevice, _frames[i].CommandPool, nullptr);
		}

		DestroySwapchain();

		vmaDestroyAllocator(_allocator);

		vkDestroyDevice(_logicalDevice, nullptr);
		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		if (_debugMessenger != VK_NULL_HANDLE)
		{
			vkb::destroy_debug_utils_messenger(_instance, _debugMessenger, nullptr);
		}

		vkDestroyInstance(_instance, nullptr);
    }

    void Renderer::InitVulkan()
    {
		const char* appName = glfwGetWindowTitle(_pWindow);

		vkb::InstanceBuilder builder;

		auto inst_ret = builder.set_app_name(appName)
			.request_validation_layers(VALIDATION_LAYERS_ENABLED)
			.set_debug_callback(messageCallback)
			.require_api_version(1, 3, 0)
			.build();


		vkb::Instance vkb_inst = inst_ret.value();
		_instance = vkb_inst.instance;
		_debugMessenger = vkb_inst.debug_messenger;

		VK_CHECK(glfwCreateWindowSurface(_instance, _pWindow, nullptr, &_surface));

		VkPhysicalDeviceFeatures features{};
		features.samplerAnisotropy = VK_TRUE;

		VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		features13.dynamicRendering = true;
		features13.synchronization2 = true;

		VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;

		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		auto physicalDevice_ret = selector
			.allow_any_gpu_device_type(false)
			.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
			.set_minimum_version(1, 3)
			.set_required_features(features)
			.set_required_features_13(features13)
			.set_required_features_12(features12)
			.set_surface(_surface)
			.select();

		vkb::PhysicalDevice vkb_physicalDevice = physicalDevice_ret.value();
		_physicalDevice = vkb_physicalDevice.physical_device;

		vkb::DeviceBuilder deviceBuilder{ vkb_physicalDevice };
		auto vkbDevice_ret = deviceBuilder.build();

		vkb::Device vkb_device = vkbDevice_ret.value();
		_logicalDevice = vkb_device.device;

		auto graphicsQueue_ret = vkb_device.get_queue(vkb::QueueType::graphics);
		_graphicsQueue = graphicsQueue_ret.value();

		auto graphicsQueueFamily_ret = vkb_device.get_queue_index(vkb::QueueType::graphics);
		_graphicsQueueFamilyIndex = graphicsQueueFamily_ret.value();

		auto presentQueue_ret = vkb_device.get_queue(vkb::QueueType::present);
		_presentQueue = presentQueue_ret.value();

		auto presentQueueFamily_ret = vkb_device.get_queue_index(vkb::QueueType::present);
		_presentQueueFamilyIndex = presentQueueFamily_ret.value();

		VmaAllocatorCreateInfo allocatorInfo
		{
			.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			.physicalDevice = _physicalDevice,
			.device = _logicalDevice,
			.instance = _instance,
		};
		vmaCreateAllocator(&allocatorInfo, &_allocator);
    }

    void Renderer::InitSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreInfo = vulkan::semaphoreCreateInfo();
        VkFenceCreateInfo fenceInfo = vulkan::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VK_CHECK(vkCreateSemaphore(_logicalDevice, &semaphoreInfo, nullptr, &_frames[i].ImageAvailableSemaphore));

            VK_CHECK(vkCreateSemaphore(_logicalDevice, &semaphoreInfo, nullptr, &_frames[i].RenderFinishedSemaphore));

            VK_CHECK(vkCreateFence(_logicalDevice, &fenceInfo, nullptr, &_frames[i].Fence));
        }

        // Immediate Sync Objects
        VK_CHECK(vkCreateFence(_logicalDevice, &fenceInfo, nullptr, &_immediateData.Fence));
    }

    void Renderer::InitSwapchain()
    {
		CreateSwapchain();
    }

    void Renderer::InitCommands()
    {
		VkCommandPoolCreateInfo poolInfo = vulkan::commandPoolCreateInfo(
			_graphicsQueueFamilyIndex, 
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VK_CHECK(vkCreateCommandPool(_logicalDevice, &poolInfo, nullptr, &_frames[i].CommandPool));

			VkCommandBufferAllocateInfo allocInfo = vulkan::commandBufferAllocateInfo(_frames[i].CommandPool, 1);
			VK_CHECK(vkAllocateCommandBuffers(_logicalDevice, &allocInfo, &_frames[i].CommandBuffer));
		}

		// Immediate Command Pool
		VK_CHECK(vkCreateCommandPool(_logicalDevice, &poolInfo, nullptr, &_immediateData.CommandPool));

		VkCommandBufferAllocateInfo immAllocInfo = vulkan::commandBufferAllocateInfo(_immediateData.CommandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(_logicalDevice, &immAllocInfo, &_immediateData.CommandBuffer));
    }

    void Renderer::InitGraphicsPipeline()
    {
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = vulkan::pipelineLayoutCreateInfo();
		VK_CHECK(vkCreatePipelineLayout(_logicalDevice, &pipelineLayoutInfo, nullptr, &_pipelineLayout));
    }

	void Renderer::InitImGui()
	{
		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo poolInfo
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			.maxSets = 1000,
			.poolSizeCount = (uint32_t)std::size(poolSizes),
			.pPoolSizes = poolSizes,
		};
		VK_CHECK(vkCreateDescriptorPool(_logicalDevice, &poolInfo, nullptr, &_imGuiDescriptorPool));

		ImGui::CreateContext();
		bool initialized = ImGui_ImplGlfw_InitForVulkan(_pWindow, true);
		if (!initialized)
		{
			throw std::exception("No fue posible inicializar la implementacion ImGui para Vulkan");
		}

		VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &_swapchain.Format,
		};

		ImGui_ImplVulkan_InitInfo initInfo
		{
			.Instance = _instance,
			.PhysicalDevice = _physicalDevice,
			.Device = _logicalDevice,
			.QueueFamily = _graphicsQueueFamilyIndex,
			.Queue = _graphicsQueue,
			.DescriptorPool = _imGuiDescriptorPool,
			.MinImageCount = 3,
			.ImageCount = 3,
			.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
			.UseDynamicRendering = true,
			.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo,
		};

		auto& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		initialized = ImGui_ImplVulkan_Init(&initInfo);
		if (!initialized)
		{
			throw std::exception("No fue posible inicializar ImGui para Vulkan");
		}

		ImGui_ImplVulkan_CreateFontsTexture();
	}

	VkPipeline Renderer::BuildGenericGraphicsPipeline(VkShaderModule vertexShaderModule, VkShaderModule fragmentShaderModule)
	{
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages
		{
			vulkan::pipelineShaderStageCreateInfo(vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT),
			vulkan::pipelineShaderStageCreateInfo(fragmentShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT),
		};

		VkFormat colorAttachmentFormat = _drawData.Image.Format;
		VkFormat depthAttachmentFormat = _drawData.DepthImage.Format;

		VkPipelineRenderingCreateInfo renderingInfo
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.pNext = nullptr,
			.viewMask = 0,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &colorAttachmentFormat,
			.depthAttachmentFormat = depthAttachmentFormat,
			.stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
		};

		VkPipelineVertexInputStateCreateInfo _vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

		VkPipelineInputAssemblyStateCreateInfo _inputAssemblyInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE,
		};

		VkPipelineViewportStateCreateInfo viewportState
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.viewportCount = 1,
			.scissorCount = 1,
		};

		VkPipelineRasterizationStateCreateInfo rasterizer
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0,
			.depthBiasClamp = 0,
			.depthBiasSlopeFactor = 0,
			.lineWidth = 1.0f,
		};

		VkPipelineMultisampleStateCreateInfo multisampling
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1.0f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE,
		};

		VkPipelineDepthStencilStateCreateInfo depthStencil
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f,
		};

		VkPipelineColorBlendAttachmentState colorBlendAttachment
		{
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};

		VkPipelineColorBlendStateCreateInfo colorBlending
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment,
		};

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
			.pNext = &renderingInfo,
			.flags = 0,
			.stageCount = (uint32_t)shaderStages.size(),
			.pStages = shaderStages.data(),
			.pVertexInputState = &_vertexInputInfo,
			.pInputAssemblyState = &_inputAssemblyInfo,
			.pTessellationState = nullptr,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicInfo,
			.layout = _pipelineLayout,
			.renderPass = VK_NULL_HANDLE,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0,
		};

		VkPipeline pipeline = VK_NULL_HANDLE;
		VK_CHECK(vkCreateGraphicsPipelines(_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
		return pipeline;
	}

    void Renderer::CreateSwapchain()
    {
        //Swapchain
		vkb::SwapchainBuilder swapchainBuilder{ _physicalDevice, _logicalDevice, _surface };

		VkSurfaceFormatKHR desiredFormat
		{
			.format = VK_FORMAT_B8G8R8A8_SRGB,
			.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};

		auto swapchain_ret = swapchainBuilder
			.set_desired_format(desiredFormat)
			.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
			.set_desired_min_image_count(3)
			.set_desired_extent(_width, _height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build();

		vkb::Swapchain vkb_swapchain = swapchain_ret.value();

		_swapchain.Handle = vkb_swapchain.swapchain;
		_swapchain.Images = vkb_swapchain.get_images().value();
		_swapchain.ImageViews = vkb_swapchain.get_image_views().value();
		_swapchain.Format = vkb_swapchain.image_format;
		_swapchain.Extent = vkb_swapchain.extent;

		//Draw Image
		if (_drawData.Image.Handle != VK_NULL_HANDLE)
		{
			vkDestroyImageView(_logicalDevice, _drawData.Image.View, nullptr);
			vmaDestroyImage(_allocator, _drawData.Image.Handle, _drawData.Image.Allocation);
			_drawData.Image.Handle = VK_NULL_HANDLE;
		}

		VkExtent3D drawImageExtent
		{
			.width = _width,
			.height = _height,
			.depth = 1,
		};

		_drawData.Image.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
		_drawData.Image.Extent = drawImageExtent;

		VkImageUsageFlags drawImageUsages = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
			| VK_IMAGE_USAGE_TRANSFER_DST_BIT
			| VK_IMAGE_USAGE_STORAGE_BIT
			| VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageCreateInfo drawImageInfo = vulkan::imageCreateInfo(_drawData.Image.Format, drawImageUsages, _drawData.Image.Extent);

		VmaAllocationCreateInfo imageAllocInfo
		{
			.usage = VMA_MEMORY_USAGE_GPU_ONLY,
			.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		};

		VK_CHECK(vmaCreateImage(_allocator, &drawImageInfo, &imageAllocInfo, &_drawData.Image.Handle, &_drawData.Image.Allocation, nullptr));

		VkImageViewCreateInfo drawImageViewInfo = vulkan::imageViewCreateInfo(
			_drawData.Image.Format, 
			_drawData.Image.Handle, 
			VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(_logicalDevice, &drawImageViewInfo, nullptr, &_drawData.Image.View));

		//Depth Image
		if (_drawData.DepthImage.Handle != VK_NULL_HANDLE)
		{
			vkDestroyImageView(_logicalDevice, _drawData.DepthImage.View, nullptr);
			vmaDestroyImage(_allocator, _drawData.DepthImage.Handle, _drawData.DepthImage.Allocation);
			_drawData.DepthImage.Handle = VK_NULL_HANDLE;
		}

		_drawData.DepthImage.Format = VK_FORMAT_D32_SFLOAT;
		_drawData.DepthImage.Extent = drawImageExtent;
		VkImageUsageFlags depthImageUsages = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VkImageCreateInfo depthImageInfo = vulkan::imageCreateInfo(_drawData.DepthImage.Format, depthImageUsages, drawImageExtent);

		VK_CHECK(vmaCreateImage(_allocator, &depthImageInfo, &imageAllocInfo, &_drawData.DepthImage.Handle, &_drawData.DepthImage.Allocation, nullptr));

		VkImageViewCreateInfo depthImageViewInfo = vulkan::imageViewCreateInfo(
			_drawData.DepthImage.Format,
			_drawData.DepthImage.Handle,
			VK_IMAGE_ASPECT_DEPTH_BIT);

		VK_CHECK(vkCreateImageView(_logicalDevice, &depthImageViewInfo, nullptr, &_drawData.DepthImage.View));
    }

	void Renderer::RecreateSwapchain()
	{
		VK_CHECK(vkDeviceWaitIdle(_logicalDevice));

		DestroySwapchain();

		CreateSwapchain();

		_resized = false;
	}

	void Renderer::DestroySwapchain()
	{
		vkDestroySwapchainKHR(_logicalDevice, _swapchain.Handle, nullptr);

		for (auto imageView : _swapchain.ImageViews) {
			vkDestroyImageView(_logicalDevice, imageView, nullptr);
		}
	}

	void Renderer::DestroyImgui()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		vkDestroyDescriptorPool(_logicalDevice, _imGuiDescriptorPool, nullptr);
	}

	void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0,
			.pInheritanceInfo = nullptr,
		};

		VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		vulkan::transitionImage(commandBuffer, _drawData.Image.Handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		DrawBackground(commandBuffer);

		vulkan::transitionImage(commandBuffer, _drawData.Image.Handle, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		vulkan::transitionImage(commandBuffer, _drawData.DepthImage.Handle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		DrawGeometry(commandBuffer);

		vulkan::transitionImage(commandBuffer, _drawData.Image.Handle, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		vulkan::transitionImage(commandBuffer, _swapchain.Images[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		vulkan::copyImage(commandBuffer, _drawData.Image.Handle, _swapchain.Images[imageIndex], _drawData.ImageExtent, _swapchain.Extent);

		vulkan::transitionImage(commandBuffer, _swapchain.Images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		DrawImGui(commandBuffer, _swapchain.ImageViews[imageIndex]);

		vulkan::transitionImage(commandBuffer, _swapchain.Images[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		
		VK_CHECK(vkEndCommandBuffer(commandBuffer));
	}

	void Renderer::DrawBackground(VkCommandBuffer commandBuffer)
	{
		for (auto& renderFunction : _backgroundRenderFunctions)
		{
			renderFunction(commandBuffer, _drawData);
		}
	}

	void Renderer::DrawGeometry(VkCommandBuffer commandBuffer)
	{
		VkRenderingAttachmentInfo colorAttachment = vulkan::colorAttachmentInfo(
			_drawData.Image.View,
			nullptr,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkRenderingAttachmentInfo depthAttachment = vulkan::depthAttachmentInfo(
			_drawData.DepthImage.View,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = vulkan::renderingInfo(
			_drawData.ImageExtent,
			&colorAttachment,
			&depthAttachment);

		vkCmdBeginRendering(commandBuffer, &renderInfo);

		
		for (auto& renderFunction : _renderFunctions)
		{
			renderFunction(commandBuffer, _drawData);
		}

		vkCmdEndRendering(commandBuffer);
	}

	void Renderer::DrawImGui(VkCommandBuffer commandBuffer, VkImageView targetImageView)
	{
		VkRenderingAttachmentInfo colorAttachment = vulkan::colorAttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingInfo renderInfo = vulkan::renderingInfo(_swapchain.Extent, &colorAttachment, nullptr);

		vkCmdBeginRendering(commandBuffer, &renderInfo);

		ImDrawData* data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(data, commandBuffer);

		vkCmdEndRendering(commandBuffer);
	}
}
