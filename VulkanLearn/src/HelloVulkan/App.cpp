#include "App.hpp"

#include <set>
#include <fstream>
#include <algorithm>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <stb/stb_image.h>

#include <tinyobjloader/tiny_obj_loader.h>

#include "../Engine/InputState.hpp"
#include "../Vulkan/Utils.hpp"


namespace std {
	template<> struct hash<HelloVulkan::Vertex> {
		size_t operator()(HelloVulkan::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

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

namespace HelloVulkan
{
	struct SwapChainSupportDetails 
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	static const std::vector<const char*> requiredLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	static  const std::vector<const char*> requiredDeviceExtensions = 
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	/*const std::vector<Vertex> _vertices = 
	{
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
		{{ 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
		{{ 0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = 
	{
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};*/

	static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice);
	bool hasStencilComponent(VkFormat format);

	static std::vector<char> readFile(const std::string& filename);
	

	App::App()
		: _camera(WIDTH, HEIGHT)
	{
	}

	void App::Run()
	{
		InitWindow();
		InitVulkan();
		Loop();
		Clean();
	}

	void App::OnScroll(double yoffset)
	{
		_camera.OnScroll((float)yoffset);
	}

	void App::InitWindow()
	{
		spdlog::info("Inicializando Ventana");

		if (glfwInit() == GLFW_FALSE)
		{
			throw std::exception("Error al inicializar glfw");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Triangle", nullptr, nullptr);
		glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetWindowUserPointer(_window, this);

		glfwSetFramebufferSizeCallback(_window, [](GLFWwindow* window, int width, int height)
			{
				auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
				app->SetResized(true);

				bool minimized = width == 0 || height == 0;
				app->SetDoRender(!minimized);
			}
		);

		glfwSetScrollCallback(_window, [](GLFWwindow* window, double xoffset, double yoffset)
			{
				auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
				app->OnScroll(yoffset);
			}
		);

		glfwSetCursorPosCallback(_window, [](GLFWwindow* window, double xpos, double ypos)
			{
				auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));

				static double lastX = xpos;
				static double lastY = ypos;

				double xoffset = xpos - lastX;
				double yoffset = lastY - ypos;

				app->GetCamera().OnMouseMove((float)xoffset, (float)yoffset);

				lastX = xpos;
				lastY = ypos;
			}
		);

		if (!_window) {
			throw std::exception("Error al crear la ventana");
		}

		spdlog::info("Inicializacion de Ventana exitosa");
	}

    void App::Loop()
    {
        double lastTime = glfwGetTime();
        int frameCount = 0;

        while (!glfwWindowShouldClose(_window)) {
            float currentFrame = static_cast<float>(glfwGetTime());
            _deltaTime = currentFrame - _lastFrame;
            _lastFrame = currentFrame;

            glfwPollEvents();

			OnUpdate(_deltaTime);

			if (_doRender)
			{
				OnRender();
			}
			else
			{
				//throttle the speed to avoid the endless spinning
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

            // Calculate FPS
            double currentTime = glfwGetTime();
            frameCount++;
            if (currentTime - lastTime >= 0.5) {
                double fps = frameCount / (currentTime - lastTime);
                std::string windowTitle = "Vulkan Triangle | FPS: " + std::to_string(fps);
                glfwSetWindowTitle(_window, windowTitle.c_str());
				_fps = fps;
                frameCount = 0;
                lastTime = currentTime;
            }
        }

        VK_CHECK(vkDeviceWaitIdle(_logicalDevice));
    }

	void App::Clean()
	{
		spdlog::info("Limpiando");

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(_logicalDevice, _renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(_logicalDevice, _imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(_logicalDevice, _inFlightFences[i], nullptr);
		}

		vkDestroyBuffer(_logicalDevice, _indexBuffer, nullptr);
		vkFreeMemory(_logicalDevice, _indexBufferMemory, nullptr);

		vkDestroyBuffer(_logicalDevice, _vertexBuffer, nullptr);
		vkFreeMemory(_logicalDevice, _vertexBufferMemory, nullptr);

		vkDestroyCommandPool(_logicalDevice, _commandPool, nullptr);

		vkDestroyPipeline(_logicalDevice, _graphicsPipeline, nullptr);

		vkDestroyPipelineLayout(_logicalDevice, _pipelineLayout, nullptr);

		vkDestroyRenderPass(_logicalDevice, _renderPass, nullptr);

		CleanUpSwapChain();

		vkDestroySampler(_logicalDevice, _textureSampler, nullptr);
		vkDestroyImageView(_logicalDevice, _textureImageView, nullptr);
		vkDestroyImage(_logicalDevice, _textureImage, nullptr);
		vkFreeMemory(_logicalDevice, _textureImageMemory, nullptr);

		vkDestroyDescriptorPool(_logicalDevice, _descriptorPool, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(_logicalDevice, _uniformBuffers[i], nullptr);
			vkFreeMemory(_logicalDevice, _uniformBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorSetLayout(_logicalDevice, _descriptorSetLayout, nullptr);

		Vulkan::BoostrapData data = {
			.instance = _instance,
			.debugMessenger = _messenger,
			.surface = _surface,
			.physicalDevice = _physicalDevice,
			.logicalDevice = _logicalDevice,
			.graphicsQueue = _graphicsQueue,
			.graphicsQueueFamilyIndex = _graphicsQueueFamilyIndex,
			.presentQueue = _presentQueue,
			.presentQueueFamilyIndex = _presentQueueFamilyIndex
		};
		Vulkan::cleanBoostrapedData(data);

		glfwDestroyWindow(_window);
		glfwTerminate();
	}

	void App::OnUpdate(float dt)
	{
		if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(_window, GLFW_TRUE);
		}

		if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS)
		{
			Engine::InputState::Up = true;
		}
		else
		{
			Engine::InputState::Up = false;
		}

		if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS)
		{
			Engine::InputState::Down = true;
		}
		else
		{
			Engine::InputState::Down = false;
		}

		if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS)
		{
			Engine::InputState::Left = true;
		}
		else
		{
			Engine::InputState::Left = false;
		}

		if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS)
		{
			Engine::InputState::Right = true;
		}
		else
		{
			Engine::InputState::Right = false;
		}

		_camera.OnUpdate(_deltaTime);
	}

	void App::OnRender()
	{
		DrawFrame();
	}

	void App::InitVulkan()
	{
		auto data = Vulkan::boostrapVulkan(_window, messageCallback, true);
		_instance = data.instance;
		_messenger = data.debugMessenger;
		_surface = data.surface;
		_physicalDevice = data.physicalDevice;
		_logicalDevice = data.logicalDevice;
		_graphicsQueue = data.graphicsQueue;
		_graphicsQueueFamilyIndex = data.graphicsQueueFamilyIndex;
		_presentQueue = data.presentQueue;
		_presentQueueFamilyIndex = data.presentQueueFamilyIndex;

		CreateCommandPool();
		CreateTextureImage(TEXTURE_PATH.c_str());
		CreateTextureImageView();
		CreateTextureSampler();
		CreateDescriptorSetLayout();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateSwapChain();
		CreateDepthResources();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFramebuffers();
		LoadModel(MODEL_PATH.c_str());
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateCommandBuffers();
		CreateSyncObjects();
	}

	void App::CreateSwapChain()
	{
		int width, height;
		glfwGetFramebufferSize(_window, &width, &height);

		auto data = Vulkan::boostrapSwapchain(width, height, _physicalDevice, _logicalDevice, _surface);
		_swapChain = data.swapchain;
		_swapChainImageFormat = data.imageFormat;
		_swapChainExtent = data.extent;
		_swapChainImages = data.images;
		_swapChainImageViews = data.imageViews;
	}

	void App::CreateRenderPass()
	{
		spdlog::info("Inicializando RenderPass");

		VkAttachmentDescription colorAttachment
		{
			.format = _swapChainImageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};

		VkAttachmentDescription depthAttachment
		{
			.format = FindDepthFormat(),
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference colorAttachmentRef
		{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference depthAttachmentRef
		{
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass
		{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef,
			.pDepthStencilAttachment = &depthAttachmentRef,
		};

		VkSubpassDependency dependency
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		};

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassInfo
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = (uint32_t)attachments.size(),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency,
		};

		VK_CHECK(vkCreateRenderPass(_logicalDevice, &renderPassInfo, nullptr, &_renderPass));
		spdlog::info("Inicializacion de RenderPass exitosa");
	}

	void App::CreateGraphicsPipeline()
	{
		spdlog::info("Inicializando Graphics pipeline");

		auto vertShaderCode = readFile("assets/shaders/shader.vert.spv");
		auto fragShaderCode = readFile("assets/shaders/shader.frag.spv");

		VkShaderModule vertexShaderModule = CreateShaderModule(vertShaderCode);
		VkShaderModule fragmentShaderModule = CreateShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertexShaderStageInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertexShaderModule,
			.pName = "main"
		};

		VkPipelineShaderStageCreateInfo fragmentShaderStageInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragmentShaderModule,
			.pName = "main"
		};

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

		std::vector<VkDynamicState> dynamicStates = 
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = (uint32_t)dynamicStates.size(),
			.pDynamicStates = dynamicStates.data(),
		};

		auto bindingDescription = Vertex::GetBindingDescription();
		auto attributeDescriptions = Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDescription,
			.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size(),
			.pVertexAttributeDescriptions = attributeDescriptions.data(),
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE,
		};

		VkViewport viewport
		{
			.x = 0.0f,
			.y = 0.0f,
			.width = (float)_swapChainExtent.width,
			.height = (float)_swapChainExtent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};

		VkRect2D scissor
		{
			.offset = { 0, 0 },
			.extent = _swapChainExtent,
		};

		VkPipelineViewportStateCreateInfo viewportState
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor
		};

		VkPipelineRasterizationStateCreateInfo rasterizer
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f,
		};

		VkPipelineMultisampleStateCreateInfo multisampling
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1.0f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE,
		};

		VkPipelineColorBlendAttachmentState colorBlendAttachment
		{
			.blendEnable = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT 
				| VK_COLOR_COMPONENT_G_BIT 
				| VK_COLOR_COMPONENT_B_BIT 
				| VK_COLOR_COMPONENT_A_BIT,
		};

		VkPipelineColorBlendStateCreateInfo colorBlending
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment,
			.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f}
		};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &_descriptorSetLayout,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr,
		};

		VK_CHECK(vkCreatePipelineLayout(_logicalDevice, &pipelineLayoutInfo, nullptr, &_pipelineLayout));

		VkPipelineDepthStencilStateCreateInfo depthStencil
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f,
		};

		VkGraphicsPipelineCreateInfo pipelineInfo
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = 2,
			.pStages = shaderStages,
			.pVertexInputState = &vertexInputInfo,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicState,
			.layout = _pipelineLayout,
			.renderPass = _renderPass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1,
		};

		VK_CHECK(vkCreateGraphicsPipelines(_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline));

		vkDestroyShaderModule(_logicalDevice, vertexShaderModule, nullptr);
		vkDestroyShaderModule(_logicalDevice, fragmentShaderModule, nullptr);

		spdlog::info("Inicializacion de Graphics pipeline exitosa");
	}

	void App::CreateFramebuffers()
	{
		spdlog::info("Inicializando Framebuffers");

		_framebuffers.resize(_swapChainImageViews.size());

		for (size_t i = 0; i < _swapChainImageViews.size(); i++) {

			std::array<VkImageView, 2> attachments = {
				_swapChainImageViews[i],
				_depthImageView
			};

			VkFramebufferCreateInfo framebufferInfo
			{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = _renderPass,
				.attachmentCount = (uint32_t)attachments.size(),
				.pAttachments = attachments.data(),
				.width = _swapChainExtent.width,
				.height = _swapChainExtent.height,
				.layers = 1,
			};

			VK_CHECK(vkCreateFramebuffer(_logicalDevice, &framebufferInfo, nullptr, &_framebuffers[i]));
		}

		spdlog::info("Inicializacion de Framebuffers exitosa");
	}

	void App::CreateCommandPool()
	{
		spdlog::info("Inicializando Command Pool");

		VkCommandPoolCreateInfo poolInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = _graphicsQueueFamilyIndex,
		};

		VK_CHECK(vkCreateCommandPool(_logicalDevice, &poolInfo, nullptr, &_commandPool));

		spdlog::info("Inicializacion de Command Pool exitosa");
	}

	void App::CreateCommandBuffers()
	{
		spdlog::info("Inicializando Command Buffer");

		_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo allocInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = _commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = (uint32_t)_commandBuffers.size(),
		};

		VK_CHECK(vkAllocateCommandBuffers(_logicalDevice, &allocInfo, _commandBuffers.data()));

		spdlog::info("Inicializacion de Command Buffer exitosa");
	}

	void App::CreateSyncObjects()
	{
		spdlog::info("Inicializando Objetos de syncronizacion");

		VkSemaphoreCreateInfo semaphoreInfo
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		VkFenceCreateInfo fenceInfo
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VK_CHECK(vkCreateSemaphore(_logicalDevice, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]));

			VK_CHECK(vkCreateSemaphore(_logicalDevice, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]));

			VK_CHECK(vkCreateFence(_logicalDevice, &fenceInfo, nullptr, &_inFlightFences[i]));
		}
		
		spdlog::info("Inicializacion de Objetos de sincronizacion exitosa");
	}

	void App::RecreateSwapChain()
	{
		VK_CHECK(vkDeviceWaitIdle(_logicalDevice));

		CleanUpSwapChain();

		CreateSwapChain();
		CreateDepthResources();
		CreateFramebuffers();
	}

	void App::CleanUpSwapChain()
	{
		vkDestroyImageView(_logicalDevice, _depthImageView, nullptr);
		vkDestroyImage(_logicalDevice, _depthImage, nullptr);
		vkFreeMemory(_logicalDevice, _depthImageMemory, nullptr);

		for (auto framebuffer : _framebuffers) {
			vkDestroyFramebuffer(_logicalDevice, framebuffer, nullptr);
		}

		for (auto imageView : _swapChainImageViews) {
			vkDestroyImageView(_logicalDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(_logicalDevice, _swapChain, nullptr);
	}

	void App::CreateVertexBuffer()
	{
		spdlog::info("Inicializando Buffer de _vertices");

		size_t size = sizeof(_vertices[0]) * _vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			size, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, 
			stagingBufferMemory);

		void* data;
		VK_CHECK(vkMapMemory(_logicalDevice, stagingBufferMemory, 0, size, 0, &data));

		memcpy(data, _vertices.data(), size);

		vkUnmapMemory(_logicalDevice, stagingBufferMemory);

		CreateBuffer(
			size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			_vertexBuffer, 
			_vertexBufferMemory);

		CopyBuffer(stagingBuffer, _vertexBuffer, size);

		vkDestroyBuffer(_logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(_logicalDevice, stagingBufferMemory, nullptr);

		spdlog::info("Inicializacion de Buffer de vertices exitosa");
	}

	void App::CreateIndexBuffer()
	{
		spdlog::info("Inicializando Buffer de Indices");

		size_t size = sizeof(_indices[0]) * _indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			size, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, 
			stagingBufferMemory);

		void* data;
		VK_CHECK(vkMapMemory(_logicalDevice, stagingBufferMemory, 0, size, 0, &data));

		memcpy(data, _indices.data(), size);

		vkUnmapMemory(_logicalDevice, stagingBufferMemory);

		CreateBuffer(
			size, 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			_indexBuffer, 
			_indexBufferMemory);

		CopyBuffer(stagingBuffer, _indexBuffer, size);

		vkDestroyBuffer(_logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(_logicalDevice, stagingBufferMemory, nullptr);

		spdlog::info("Inicializacion de Buffer de Indices exitosa");
	}

	void App::CreateDescriptorSetLayout()
	{
		spdlog::info("Inicializando Descriptor Set Layout");

		VkDescriptorSetLayoutBinding uboLayoutBinding
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = nullptr,
		};

		VkDescriptorSetLayoutBinding samplerLayoutBinding
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		};

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = (uint32_t)bindings.size(),
			.pBindings = bindings.data(),
		};

		VK_CHECK(vkCreateDescriptorSetLayout(_logicalDevice, &layoutInfo, nullptr, &_descriptorSetLayout));

		spdlog::info("Inicializacion de Descriptor Set Layout exitosa");
	}

	void App::CreateUniformBuffers()
	{
		size_t size = sizeof(UniformBufferObject); 

		_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			CreateBuffer(
				size,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				_uniformBuffers[i], 
				_uniformBuffersMemory[i]);

			VK_CHECK(vkMapMemory(_logicalDevice, _uniformBuffersMemory[i], 0, size, 0, &_uniformBuffersMapped[i]));
		}
	}

	void App::CreateDescriptorPool()
	{
		spdlog::info("Inicializando Descriptor Pool");

		VkDescriptorPoolSize uboPoolSize
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
		};

		VkDescriptorPoolSize samplerPoolSize
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
		};

		std::array<VkDescriptorPoolSize, 2> poolSizes = { uboPoolSize, samplerPoolSize };

		VkDescriptorPoolCreateInfo poolInfo
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = (uint32_t) MAX_FRAMES_IN_FLIGHT,
			.poolSizeCount = (uint32_t)poolSizes.size(),
			.pPoolSizes = poolSizes.data(),
		};

		VK_CHECK(vkCreateDescriptorPool(_logicalDevice, &poolInfo, nullptr, &_descriptorPool));

		spdlog::info("Inicializacion de Descriptor Pool exitosa");
	}

	void App::CreateDescriptorSets()
	{
		spdlog::info("Inicializando Descriptor Sets");

		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = _descriptorPool,
			.descriptorSetCount = (uint32_t)MAX_FRAMES_IN_FLIGHT,
			.pSetLayouts = layouts.data(),
		};

		_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		VK_CHECK(vkAllocateDescriptorSets(_logicalDevice, &allocInfo, _descriptorSets.data()));

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo bufferInfo
			{
				.buffer = _uniformBuffers[i],
				.offset = 0,
				.range = sizeof(UniformBufferObject),
			};

			VkDescriptorImageInfo imageInfo
			{
				.sampler = _textureSampler,
				.imageView = _textureImageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			VkWriteDescriptorSet bufferDescriptorWrite
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = _descriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &bufferInfo,
			};

			VkWriteDescriptorSet imageDescriptorWrite
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = _descriptorSets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfo,
			};

			std::array<VkWriteDescriptorSet, 2> descriptorWrites = { bufferDescriptorWrite, imageDescriptorWrite };

			vkUpdateDescriptorSets(_logicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}

		spdlog::info("Inicializacion de Descriptor Sets exitosa");

	}

	void App::CreateTextureImage(const char* path)
	{
		spdlog::info("Inicializando Imagen de Textura de escultura");

		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::exception("No fue posible cargar la textura");
		}
		
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			imageSize, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, 
			stagingBufferMemory);

		void* data;
		VK_CHECK(vkMapMemory(_logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data));

		memcpy(data, pixels, static_cast<size_t>(imageSize));

		vkUnmapMemory(_logicalDevice, stagingBufferMemory);

		stbi_image_free(pixels);

		CreateImage({.width = (uint32_t)texWidth,
				.height = (uint32_t)texHeight,
				.format = VK_FORMAT_R8G8B8A8_SRGB,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				.image = _textureImage,
				.imageMemory = _textureImageMemory});

		TransitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(stagingBuffer, _textureImage, (uint32_t)texWidth, (uint32_t)texHeight);

		TransitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(_logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(_logicalDevice, stagingBufferMemory, nullptr);

		spdlog::info("Inicializacion de Imagen de Textura de escultura exitosa");
	}

	void App::CreateTextureImageView()
	{
		spdlog::info("Inicializando Vista de Imagen de Textura");

		_textureImageView = CreateImageView(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

		spdlog::info("Inicializacion de Vista de Imagen de Textura exitosa");
	}

	void App::CreateTextureSampler()
	{
		spdlog::info("Inicializando Muestreador de Textura");

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(_physicalDevice, &properties);

		VkSamplerCreateInfo samplerInfo
		{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.minLod = 0.0f,
			.maxLod = 0.0f,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE,
		};

		VK_CHECK(vkCreateSampler(_logicalDevice, &samplerInfo, nullptr, &_textureSampler));

		spdlog::info("Inicializacion de Muestreador de Textura exitosa");
	}

	void App::LoadModel(const char* path)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path);
		if (!loaded)
		{
			throw std::exception("No fue posible cargar el modelo");
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.color = { 1.0f, 1.0f, 1.0f };

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = (uint32_t)_vertices.size();
					_vertices.push_back(vertex);
				}

				_indices.push_back(uniqueVertices[vertex]);
			}
		}
	}

	void App::CreateDepthResources()
	{
		spdlog::info("Inicializando Recursos de Profundidad");

		VkFormat depthFormat = FindDepthFormat();
		CreateImage({ .width = _swapChainExtent.width,
					.height = _swapChainExtent.height,
					.format = depthFormat,
					.tiling = VK_IMAGE_TILING_OPTIMAL,
					.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
					.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					.image = _depthImage,
					.imageMemory = _depthImageMemory });

		_depthImageView = CreateImageView(_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		TransitionImageLayout(_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		spdlog::info("Inicializacion de Recursos de Profundidad exitosa");
	}

	void App::DrawFrame()
	{
		VK_CHECK(vkWaitForFences(_logicalDevice, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX));

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(_logicalDevice, _swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::exception("No fue posible obtener la imagen");
		}

		VK_CHECK(vkResetFences(_logicalDevice, 1, &_inFlightFences[_currentFrame]));

		vkResetCommandBuffer(_commandBuffers[_currentFrame], 0);

		RecordCommandBuffer(_commandBuffers[_currentFrame], imageIndex);

		UpdateUniformBuffer(_currentFrame);

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &_imageAvailableSemaphores[_currentFrame],
			.pWaitDstStageMask = waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &_commandBuffers[_currentFrame],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &_renderFinishedSemaphores[_currentFrame],
		};

		VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _inFlightFences[_currentFrame]));

		VkSwapchainKHR swapChains[] = { _swapChain };
		VkPresentInfoKHR presentInfo
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &_renderFinishedSemaphores[_currentFrame],
			.swapchainCount = 1,
			.pSwapchains = swapChains,
			.pImageIndices = &imageIndex,
			.pResults = nullptr,
		};

		result = vkQueuePresentKHR(_presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
			_framebufferResized = false;
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::exception("No fue posible presentar en la cola de presentacion");
		}

		_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	VkShaderModule App::CreateShaderModule(const std::vector<char>& code)
	{
		spdlog::info("Inicializando Modulo shader");

		VkShaderModuleCreateInfo createInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = code.size(),
			.pCode = reinterpret_cast<const uint32_t*>(code.data()),
		};

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VK_CHECK(vkCreateShaderModule(_logicalDevice, &createInfo, nullptr, &shaderModule));

		spdlog::info("Inicializado Modulo shader correctamente");

		return shaderModule;
	}

	void App::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0,
			.pInheritanceInfo = nullptr,
		};

		VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.005f, 0.005f, 0.005f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassInfo
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = _renderPass,
			.framebuffer = _framebuffers[imageIndex],
			.renderArea
			{
				.offset = {0, 0},
				.extent = _swapChainExtent,
			},
			.clearValueCount = (uint32_t)clearValues.size(),
			.pClearValues = clearValues.data(),
		};

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

		VkViewport viewport
		{
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(_swapChainExtent.width),
			.height = static_cast<float>(_swapChainExtent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor
		{
			.offset = { 0, 0 },
			.extent = _swapChainExtent,
		};
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = { _vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, _indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(
			commandBuffer, 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			_pipelineLayout, 
			0, 
			1, 
			&_descriptorSets[_currentFrame], 
			0, 
			nullptr);

		vkCmdDrawIndexed(commandBuffer, (uint32_t)_indices.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		VK_CHECK(vkEndCommandBuffer(commandBuffer));
	}

	void App::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		spdlog::info("Inicializando Buffer de tamaño: {0}", size);

		VkBufferCreateInfo bufferInfo
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		VK_CHECK(vkCreateBuffer(_logicalDevice, &bufferInfo, nullptr, &buffer));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(_logicalDevice, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, _physicalDevice)
		};

		VK_CHECK(vkAllocateMemory(_logicalDevice, &allocInfo, nullptr, &bufferMemory));

		VK_CHECK(vkBindBufferMemory(_logicalDevice, buffer, bufferMemory, 0));

		spdlog::info("Inicializacion de Buffer exitosa");
	}

	void App::CreateImage(CreateImageParams params)
	{
		VkImageCreateInfo imageInfo
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = params.format,
			.extent
			{
				.width = params.width,
				.height = params.height,
				.depth = 1,
			},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = params.tiling,
			.usage = params.usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VK_CHECK(vkCreateImage(_logicalDevice, &imageInfo, nullptr, &params.image));

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(_logicalDevice, params.image, &memRequirements);

		VkMemoryAllocateInfo allocInfo
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, params.properties, _physicalDevice),
		};

		VK_CHECK(vkAllocateMemory(_logicalDevice, &allocInfo, nullptr, &params.imageMemory));

		VK_CHECK(vkBindImageMemory(_logicalDevice, params.image, params.imageMemory, 0));
	}

	void App::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = StartTemporaryCommandBuffer();

		VkBufferCopy copyRegion
		{
			.srcOffset = 0,
			.dstOffset = 0,
			.size = size,
		};
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		EndTemporaryCommandBuffer(commandBuffer);
	}

	void App::UpdateUniformBuffer(size_t currentImage)
	{
		if (_framebufferResized)
		{
			_camera.Resize(_swapChainExtent.width, _swapChainExtent.height);
		}

		UniformBufferObject ubo
		{
			.model = glm::mat4(1.0f),
			.view = _camera.GetViewMatrix(),
			.proj = _camera.GetProjectionMatrix(),
		};
		ubo.proj[1][1] *= -1;

		memcpy(_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	}

	void App::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = StartTemporaryCommandBuffer();

		VkImageMemoryBarrier barrier
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = 0,
			.dstAccessMask = 0,
			.oldLayout = oldLayout,
			.newLayout = newLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else {
			throw std::exception("Transicion de imagen no soportada");
		}

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (hasStencilComponent(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, 
			&barrier
		);

		EndTemporaryCommandBuffer(commandBuffer);
	}

	void App::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = StartTemporaryCommandBuffer();
		
		VkBufferImageCopy region
		{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.imageOffset = { 0, 0, 0 },
			.imageExtent = { width, height, 1 },
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		EndTemporaryCommandBuffer(commandBuffer);
	}

	VkImageView App::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo viewInfo
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.subresourceRange
			{
				.aspectMask = aspectFlags,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		VkImageView imageView = VK_NULL_HANDLE;
		VK_CHECK(vkCreateImageView(_logicalDevice, &viewInfo, nullptr, &imageView));

		return imageView;
	}

	VkFormat App::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(_physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::exception("No se encontro un formato soportado");
	}

	VkFormat App::FindDepthFormat()
	{
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	VkCommandBuffer App::StartTemporaryCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = _commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		VkCommandBuffer commandBuffer;
		VK_CHECK(vkAllocateCommandBuffers(_logicalDevice, &allocInfo, &commandBuffer));

		VkCommandBufferBeginInfo beginInfo
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};

		VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
		
		return commandBuffer;
	}

	void App::EndTemporaryCommandBuffer(VkCommandBuffer commandBuffer)
	{

		VK_CHECK(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer,
		};

		VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

		VK_CHECK(vkQueueWaitIdle(_graphicsQueue));

		vkFreeCommandBuffers(_logicalDevice, _commandPool, 1, &commandBuffer);
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::exception("No se encontro un tipo de memoria adecuado");
	}

	bool hasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	std::vector<char> readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			spdlog::error("No se pudo abrir el archivo {0}", filename);
			throw std::exception("No se pudo abrir el archivo");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}


}