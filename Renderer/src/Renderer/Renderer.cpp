#include "Renderer/Renderer.h"

#include <VkBootstrap.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <filesystem>

#include "Renderer/Vertex.h"
#include "Renderer/Mesh/Triangle.h"
#include "Renderer/Mesh/Square.h"
#include "Renderer/Mesh/Cube.h"

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
		std::filesystem::path currentPath = std::filesystem::current_path();
		spdlog::info("\tRenderer Working Dir: {}", currentPath.string());
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

	void Renderer::OnImGuiRender(ImGuiRenderFn imguiRenderFuntion, bool showRendererWindow)
	{
		VL_PROFILE_FUNCTION();
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

		if (showRendererWindow)
		{
			ImGui::Begin("Renderer");
			ImGui::Text("Clear Color {%.2f, %.2f, %.2f}", _clearColor.r, _clearColor.g, _clearColor.b);
			ImGui::Text("Buffer max size bytes %d", BUFFER_MAX_SIZE_BYTES);
			ImGui::Text("Chunk size bytes %d", CHUNK_SIZE_BYTES);
			ImGui::Text("Cube max count %d", CUBE_MAX_COUNT);
			ImGui::Text("Cube count per chunk %d", CUBE_COUNT_PER_CHUNK);
			ImGui::End();
		}

		ImGui::End();

		ImGui::Render();
	}

    void Renderer::Init()
	{
		VL_PROFILE_BEGIN_SESSION("Renderer Init", "init.vlprof.json");
		InitVulkan();
		InitSyncObjects();
		InitSwapchain();
		InitCommands();
		InitDescriptorPool();
		GenerateTransformsStorageBuffer();
		InitStorageDescriptors();
		InitGraphicsPipeline();
		InitComputePipeline();
		InitImGui();
		UploadMesh();

		VL_PROFILE_END_SESSION();
    }

    void Renderer::DrawFrame(const CameraUniformData& cameraUniformData)
    {
		UpdateTransformsStorage();
		UpdateCameraUniformBuffer(cameraUniformData);
		ExecuteComputeShader();

		Frame& currentFrame = CurrentFrame();
		VkCommandBuffer currentCommandBuffer = currentFrame.CommandBuffer;
		uint32_t imageIndex;

		{
			VL_PROFILE_SCOPE("Pre RecordCommandBuffer");
			VK_CHECK(vkWaitForFences(_logicalDevice, 1, &currentFrame.Fence, VK_TRUE, UINT64_MAX));

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

			VK_CHECK(vkResetCommandBuffer(currentCommandBuffer, 0));
		}


		RecordCommandBuffer(currentCommandBuffer, imageIndex);

		{
			VL_PROFILE_SCOPE("Post RecordCommandBuffer");
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

			VkResult result = vkQueuePresentKHR(_presentQueue, &presentInfo);
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _resized) {
				_resized = true;
				RecreateSwapchain();
			}
			else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
				throw std::exception("No fue posible presentar en la cola de presentacion");
			}

			_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
			_cubes.ClearUpdatedIndices();
		}
    }

	void Renderer::StartFrame(const glm::mat4& vpMatrix)
	{
		VL_PROFILE_FUNCTION();
		_vpMatrix = vpMatrix;

		if (_firstRender)
		{
			AllocateDescriptorSets(1);
		}
	}

	void Renderer::SubmitFrame(const CameraUniformData& cameraUniformData)
	{
		VL_PROFILE_FUNCTION();
		DrawFrame(cameraUniformData);

		if (_firstRender)
		{
			_firstRender = false;
		}
	}

	void Renderer::SetClearColor(glm::vec4 clearColor)
	{
		_clearColor = clearColor;
	}

    void Renderer::Shutdown()
    {
		VL_PROFILE_BEGIN_SESSION("Renderer Shutdown", "shutdown.vlprof.json");
		{
			VL_PROFILE_FUNCTION();

			VK_CHECK(vkDeviceWaitIdle(_logicalDevice));

			vmaDestroyBuffer(_allocator, _vertexBuffer.Buffer, _vertexBuffer.Allocation);
			vmaDestroyBuffer(_allocator, _indexBuffer.Buffer, _indexBuffer.Allocation);\

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
			{
				vmaDestroyBuffer(_allocator, _frames[i].CubesStagingBuffer.Buffer, _frames[i].CubesStagingBuffer.Allocation);
				vmaDestroyBuffer(_allocator, _frames[i].CubesStorageBuffer.Buffer, _frames[i].CubesStorageBuffer.Allocation);
				vmaDestroyBuffer(_allocator, _frames[i].VisibleCubesStorageBuffer.Buffer, _frames[i].VisibleCubesStorageBuffer.Allocation);
				vmaDestroyBuffer(_allocator, _frames[i].IndirectDrawBuffer.Buffer, _frames[i].IndirectDrawBuffer.Allocation);
				vmaDestroyBuffer(_allocator, _frames[i].CameraUniformBuffer.Buffer, _frames[i].CameraUniformBuffer.Allocation);
			}

			vkDestroyDescriptorSetLayout(_logicalDevice, _storageDescriptorSetLayout, nullptr);
			vkDestroyDescriptorPool(_logicalDevice, _descriptorPool, nullptr);

			DestroyImgui();
		
			vkDestroyPipeline(_logicalDevice, _graphicsPipeline, nullptr);
			vkDestroyPipeline(_logicalDevice, _computePipeline, nullptr);

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

		VL_PROFILE_END_SESSION();
    }

    void Renderer::InitVulkan()
    {
		VL_PROFILE_FUNCTION();
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

		//Get storage buffer max size
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(_physicalDevice, &deviceProperties);
		VkDeviceSize maxStorageBufferSize = deviceProperties.limits.maxStorageBufferRange;
		spdlog::info("Max Storage Buffer Size: {0}", maxStorageBufferSize);

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
		VkPushConstantRange pushConstantRange
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(PushConstant),
		};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = vulkan::pipelineLayoutCreateInfo();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		pipelineLayoutInfo.pSetLayouts = &_storageDescriptorSetLayout;
		pipelineLayoutInfo.setLayoutCount = 1;

		VK_CHECK(vkCreatePipelineLayout(_logicalDevice, &pipelineLayoutInfo, nullptr, &_pipelineLayout));
		
		_vertexShader = Shader::Create("VertexShader", "Renderer/assets/shaders/shader.vert", ShaderType::Vertex);
		_fragmentShader = Shader::Create("FragmentShader", "Renderer/assets/shaders/shader.frag", ShaderType::Fragment);
		_computeShader = Shader::Create("ComputeShader", "Renderer/assets/shaders/shader.comp", ShaderType::Compute);

		VkShaderModule vertexShaderModule = _vertexShader->CreateShaderModule(_logicalDevice);
		VkShaderModule fragmentShaderModule = _fragmentShader->CreateShaderModule(_logicalDevice);

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

		auto bindingDesc = Vertex::bindingDescription();
		VkPipelineVertexInputStateCreateInfo _vertexInputInfo = 
		{ 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDesc,
			.vertexAttributeDescriptionCount = (uint32_t)Vertex::attributeDescriptions().size(),
			.pVertexAttributeDescriptions = Vertex::attributeDescriptions().data(),
		};

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

		VK_CHECK(vkCreateGraphicsPipelines(_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline));

		vkDestroyShaderModule(_logicalDevice, vertexShaderModule, nullptr);
		vkDestroyShaderModule(_logicalDevice, fragmentShaderModule, nullptr);
    }

	void Renderer::InitComputePipeline()
	{
		VkShaderModule computeShaderModule = _computeShader->CreateShaderModule(_logicalDevice);

		VkPipelineShaderStageCreateInfo computeShaderStageInfo
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = computeShaderModule,
			.pName = "main",
		};

		VkComputePipelineCreateInfo pipelineInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = computeShaderStageInfo,
			.layout = _pipelineLayout,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0,
		};

		VK_CHECK(vkCreateComputePipelines(_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_computePipeline));

		vkDestroyShaderModule(_logicalDevice, computeShaderModule, nullptr);
	}

	void Renderer::InitDescriptorPool()
	{
		uint32_t maxStorageSetCount = 1000;
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSize.descriptorCount = maxStorageSetCount;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = maxStorageSetCount;

		VK_CHECK(vkCreateDescriptorPool(_logicalDevice, &poolInfo, nullptr, &_descriptorPool));
	}

	void Renderer::InitImGui()
	{
		VL_PROFILE_FUNCTION();
		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
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
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable
			| ImGuiConfigFlags_NoMouseCursorChange;

		initialized = ImGui_ImplVulkan_Init(&initInfo);
		if (!initialized)
		{
			throw std::exception("No fue posible inicializar ImGui para Vulkan");
		}

		ImGui_ImplVulkan_CreateFontsTexture();
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

	void Renderer::ExecuteComputeShader()
	{
		VL_PROFILE_FUNCTION();
		ImmediateSubmit([this](VkCommandBuffer commandBuffer)
		{
			vkCmdFillBuffer(commandBuffer, _frames[_currentFrame].IndirectDrawBuffer.Buffer, 0, sizeof(VkDrawIndexedIndirectCommand), 0);

			Frame& currentFrame = CurrentFrame();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _computePipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout, 0, 1, &_frames[_currentFrame].CubesDescriptorSet, 0, nullptr);
			
			uint32_t instanceCount = (uint32_t)CUBE_MAX_COUNT;
			vkCmdDispatch(commandBuffer, (instanceCount + 255) / 256, 1, 1);
		});
	}

	void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VL_PROFILE_FUNCTION();
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
		VL_PROFILE_FUNCTION();
		VkClearColorValue clearColor
		{ 
			{
				_clearColor.r,
				_clearColor.g,
				_clearColor.b,
				_clearColor.a
			}
		};
		VkImageSubresourceRange clearRange = vl::core::vulkan::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

		vkCmdClearColorImage(commandBuffer, _drawData.Image.Handle, VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);
	}

	void Renderer::InitStorageDescriptors()
	{
		VkDescriptorSetLayoutBinding layoutBinding = {};
		layoutBinding.binding = 0;
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding layoutBinding2 = {};
		layoutBinding2.binding = 1;
		layoutBinding2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBinding2.descriptorCount = 1;
		layoutBinding2.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding indirectBinding = {};
		indirectBinding.binding = 2;
		indirectBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		indirectBinding.descriptorCount = 1;
		indirectBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding cameraUniformBuffer = {};
		cameraUniformBuffer.binding = 3;
		cameraUniformBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		cameraUniformBuffer.descriptorCount = 1;
		cameraUniformBuffer.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

		std::vector<VkDescriptorSetLayoutBinding> bindings = { 
			layoutBinding, 
			layoutBinding2, 
			indirectBinding,
			cameraUniformBuffer
		};

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = (uint32_t)bindings.size();
		layoutInfo.pBindings = bindings.data();

		VK_CHECK(vkCreateDescriptorSetLayout(_logicalDevice, &layoutInfo, nullptr, &_storageDescriptorSetLayout));
	}

	void Renderer::AllocateDescriptorSets(uint32_t count)
	{
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = _descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &_storageDescriptorSetLayout;

			VK_CHECK(vkAllocateDescriptorSets(_logicalDevice, &allocInfo, &_frames[i].CubesDescriptorSet));

			std::vector<VkBuffer> bufferArray = { 
				_frames[i].CubesStorageBuffer.Buffer, 
				_frames[i].VisibleCubesStorageBuffer.Buffer,
				_frames[i].IndirectDrawBuffer.Buffer,
				_frames[i].CameraUniformBuffer.Buffer,

			};
			UpdateStorageDescriptorSet(_frames[i].CubesDescriptorSet, bufferArray);
		}
	}

	void Renderer::UpdateStorageDescriptorSet(VkDescriptorSet descriptorSet, const std::vector<VkBuffer>& bufferArray) const
	{
		std::vector<VkDescriptorBufferInfo> bufferInfos(bufferArray.size());
		
		for (size_t i = 0; i < bufferArray.size(); i++)
		{
			bufferInfos[i].buffer = bufferArray[i];
			bufferInfos[i].offset = 0;
			bufferInfos[i].range = VK_WHOLE_SIZE;
		}

		std::vector<VkWriteDescriptorSet> descriptorWrites(bufferArray.size());
		for (int i = 0; i < bufferArray.size(); ++i) {
			descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[i].dstSet = descriptorSet;
			descriptorWrites[i].dstBinding = i;
			descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[i].descriptorCount = 1;
			descriptorWrites[i].pBufferInfo = &bufferInfos[i];
		}

		vkUpdateDescriptorSets(_logicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

    void Renderer::DrawGeometry(VkCommandBuffer commandBuffer)
    {
		VL_PROFILE_FUNCTION();

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

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

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

        VkBuffer vertexBuffers[] = { _vertexBuffer.Buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, _indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT16);

		{
			VL_PROFILE_SCOPE("vkCmdBindDescriptorSets");
			vkCmdBindDescriptorSets(commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				_pipelineLayout,
				0,
				1,
				&CurrentFrame().CubesDescriptorSet,
				0,
				nullptr);
		}

		{
			VL_PROFILE_SCOPE("vkCmdPushConstants");

			PushConstant pushConstant
			{
				.transform = _vpMatrix,
			};
			vkCmdPushConstants(commandBuffer, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant), &pushConstant);
		}

		{
			VL_PROFILE_SCOPE("vkCmdDrawIndexedIndirect");
			vkCmdDrawIndexedIndirect(commandBuffer, _frames[_currentFrame].IndirectDrawBuffer.Buffer, 0, 1, sizeof(VkDrawIndexedIndirectCommand));
		}

        vkCmdEndRendering(commandBuffer);
    }

	void Renderer::DrawImGui(VkCommandBuffer commandBuffer, VkImageView targetImageView)
	{
		VL_PROFILE_FUNCTION();
		VkRenderingAttachmentInfo colorAttachment = vulkan::colorAttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingInfo renderInfo = vulkan::renderingInfo(_swapchain.Extent, &colorAttachment, nullptr);

		vkCmdBeginRendering(commandBuffer, &renderInfo);

		ImDrawData* data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(data, commandBuffer);

		vkCmdEndRendering(commandBuffer);
	}

	void Renderer::UploadMesh()
	{
		VL_PROFILE_FUNCTION();
		mesh::Cube mesh;
		const size_t vertexBufferSize = mesh.vertices.size() * sizeof(Vertex);
		const size_t indexBufferSize = mesh.indices.size() * sizeof(uint16_t);

		_vertexBuffer = CreateBuffer(vertexBufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
			| VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);

		_indexBuffer = CreateBuffer(indexBufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT
			| VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY);

		vulkan::AllocatedBuffer staging = CreateBuffer(vertexBufferSize + indexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_CPU_ONLY);

		//Stagin Buffer
		void* data = staging.Allocation->GetMappedData();
		memcpy(data, mesh.vertices.data(), vertexBufferSize);
		memcpy((char*)data + vertexBufferSize, mesh.indices.data(), indexBufferSize);

		ImmediateSubmit([&](VkCommandBuffer cmd) 
			{
				VkBufferCopy vertexCopy{ 0 };
				vertexCopy.dstOffset = 0;
				vertexCopy.srcOffset = 0;
				vertexCopy.size = vertexBufferSize;
				vkCmdCopyBuffer(cmd, staging.Buffer, _vertexBuffer.Buffer, 1, &vertexCopy);


				VkBufferCopy indexCopy{ 0 };
				indexCopy.dstOffset = 0;
				indexCopy.srcOffset = vertexBufferSize;
				indexCopy.size = indexBufferSize;
				vkCmdCopyBuffer(cmd, staging.Buffer, _indexBuffer.Buffer, 1, &indexCopy);
			}
		);

		vmaDestroyBuffer(_allocator, staging.Buffer, staging.Allocation);
	}

	void Renderer::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const
	{
		VL_PROFILE_FUNCTION();
		VK_CHECK(vkResetFences(_logicalDevice, 1, &_immediateData.Fence));
		VK_CHECK(vkResetCommandBuffer(_immediateData.CommandBuffer, 0));

		VkCommandBuffer cmd = _immediateData.CommandBuffer;

		VkCommandBufferBeginInfo beginInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr,
		};

		VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

		function(cmd);

		VK_CHECK(vkEndCommandBuffer(cmd));

		VkCommandBufferSubmitInfo cmdinfo = vulkan::commandBufferSubmitInfo(cmd);
		VkSubmitInfo2 submit = vulkan::submitInfo2(cmdinfo, nullptr, nullptr);

		VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immediateData.Fence));

		VK_CHECK(vkWaitForFences(_logicalDevice, 1, &_immediateData.Fence, true, UINT64_MAX));
	}

	void Renderer::UpdateTransformsStorage() const
	{
		VL_PROFILE_FUNCTION();
		auto& updated = _cubes.UpdatedIndices();
		size_t size = updated.size() * sizeof(CubeRenderData);
		if (size == 0)
		{
			return;
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkBuffer stagingBuffer = _frames[i].CubesStagingBuffer.Buffer;
			VkBuffer storageBuffer = _frames[i].CubesStorageBuffer.Buffer;

			const CubeRenderData* cubeData = _cubes.Data();
			CubeRenderData* mappedData = (CubeRenderData*)_frames[i].CubesStagingBuffer.Allocation->GetMappedData();

			std::vector<VkBufferCopy> copies(updated.size());
			for (size_t i = 0; i < updated.size(); i++)
			{
				size_t index = updated[i];
				mappedData[index] = cubeData[index];

				VkBufferCopy copy
				{
					.srcOffset = index * sizeof(CubeRenderData),
					.dstOffset = index * sizeof(CubeRenderData),
					.size = sizeof(CubeRenderData),
				};
				copies[i] = copy;
			}


			if (_firstRender)
			{
				ImmediateSubmit([&](VkCommandBuffer cmd){
						VkBufferCopy copy
						{
							.srcOffset = 0,
							.dstOffset = 0,
							.size = BUFFER_MAX_SIZE_BYTES,
						};

						vkCmdCopyBuffer(cmd, stagingBuffer, storageBuffer, 1, &copy);
					}
				);
			}
			else
			{
				ImmediateSubmit([&](VkCommandBuffer cmd){
						vkCmdCopyBuffer(cmd, stagingBuffer, storageBuffer, (uint32_t)copies.size(), copies.data());
					}
				);
			}

			
		}
	}

	void Renderer::UpdateCameraUniformBuffer(const CameraUniformData& cameraUniformData) const
	{
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			CameraUniformData* cameraStagingBufferData = (CameraUniformData*)_frames[i].CubesStagingBuffer.Allocation->GetMappedData();
			cameraStagingBufferData->view = cameraUniformData.view;
			cameraStagingBufferData->proj = cameraUniformData.proj;
			cameraStagingBufferData->viewProj = cameraUniformData.viewProj;
			cameraStagingBufferData->cameraPos = cameraUniformData.cameraPos;
			cameraStagingBufferData->renderDistance = cameraUniformData.renderDistance;

			VkBuffer stagingCameraBuffer = _frames[i].CubesStagingBuffer.Buffer;
			VkBuffer cameraUniformBuffer = _frames[i].CameraUniformBuffer.Buffer;

			ImmediateSubmit([&](VkCommandBuffer cmd) 
				{
					VkBufferCopy copy
					{
						.srcOffset = 0,
						.dstOffset = 0,
						.size = sizeof(CameraUniformData),
					};
					vkCmdCopyBuffer(cmd, stagingCameraBuffer, cameraUniformBuffer, 1, &copy);
				}
			);
		}
	}

	void Renderer::GenerateTransformsStorageBuffer()
	{
		VL_PROFILE_FUNCTION();
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			_frames[i].CubesStagingBuffer = CreateBuffer(
				BUFFER_MAX_SIZE_BYTES,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU);

			_frames[i].CubesStorageBuffer = CreateBuffer(
				BUFFER_MAX_SIZE_BYTES,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY);

			_frames[i].VisibleCubesStorageBuffer = CreateBuffer(
				BUFFER_MAX_SIZE_BYTES,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY);

			_frames[i].IndirectDrawBuffer = CreateBuffer(
				sizeof(VkDrawIndexedIndirectCommand),
				VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY);

			_frames[i].CameraUniformBuffer = CreateBuffer(
				sizeof(CameraUniformData),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY);
		}
	}

	vulkan::AllocatedBuffer Renderer::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
	{
		VkBufferCreateInfo bufferInfo
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.size = allocSize,
			.usage = usage,
		};

		VmaAllocationCreateInfo vmaallocInfo
		{
			.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
				.usage = memoryUsage,
		};

		vulkan::AllocatedBuffer newBuffer;
		VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &newBuffer.Buffer, &newBuffer.Allocation, &newBuffer.Info));

		return newBuffer;
	}
}
