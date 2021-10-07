#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <set>
#include <fstream>
#include <algorithm>
#include <array>

#include <chrono>

//// TODO: use glm as math library for now, this lib may be replaced or re-implement later.
typedef glm::vec2 Vector2;
typedef glm::vec3 Vector3;
typedef glm::vec4 Vector4;
typedef glm::mat4 Matrix4;
////

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char* APPNAME = "VINCI";

const int MAX_FRAMES_IN_SWAPCHAIN = 2;

const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

template <typename T>
static inline void ZeroVkStructure(T& vkStruct, uint32_t flag)
{
	static_assert(offsetof(T, sType) == 0, "Assume sType is the first member in struct.");
	static_assert(sizeof(T::sType) == sizeof(int32_t), "Assume sType is compatible with int32.");
	(int32_t&)vkStruct.sType = flag;
	memset(((uint8_t*)&vkStruct) + sizeof(flag), 0, sizeof(T) - sizeof(flag));
}

struct Vertex
{
	//Vertex()
	//{
	//	InputDescription.binding = 0;
	//	InputDescription.stride = sizeof(Vertex);
	//	InputDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	//
	//}

	Vector2 position;
	Vector3 color;
	Vector2 texCoord;
	//static VkVertexInputBindingDescription InputDescription;

	static VkVertexInputBindingDescription getDescription()
	{
		VkVertexInputBindingDescription desc = {};
		desc.binding = 0;
		desc.stride = sizeof(Vertex);
		desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return desc;
	}

	// for only position and color in Vertex
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescription()
	{
		std::array<VkVertexInputAttributeDescription, 3> ret = {};
		ret[0].location = 0;
		ret[0].binding = 0;
		ret[0].format = VK_FORMAT_R32G32_SFLOAT;
		ret[0].offset = offsetof(Vertex, position);

		ret[1] = {1 ,0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) };
		ret[2] = { 2 ,0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord) };

		return ret;
	}
};

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool IsComplete()
	{
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
};

struct SwapChainSupportDetail
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBuffer
{
    Matrix4 model;
    Matrix4 view;
    Matrix4 projection;
};

// interleaving vertex attributes
const std::vector<Vertex> DummyVertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{ 0.5f, 0.5f }, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f }, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

//// uint8 need extra extension to support, check if VkPhysicalDeviceIndexTypeUint8FeaturesEXT enabled.
// Be ware of the range, it will also indicated with VK flag when bind index buffer
const std::vector<uint8_t> DummyIndices = {
	0, 1, 2, 2, 3, 0
};

#ifdef _DEBUG
/// Validation Layer should be abstracted, but leave it here for learning usage
/// We can learn how to make validation configuration by vk_layer_settings.txt
/// Currently use default setting in this solution. 
const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

bool checkValidationLayerSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> layerAvailable(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layerAvailable.data());

	for (const char* layer : VALIDATION_LAYERS)
	{
		bool ret = false;
		for (const VkLayerProperties& availableLayer : layerAvailable)
		{
			if (strcmp(layer, availableLayer.layerName) == 0)
			{
				ret = true;
				break;
			}
		}

		if (!ret)
		{
			return false;
		}
	}
	return true;
}

VkResult createDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{

	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}
#endif

static std::vector<char> ReadFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file..");
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> readBuffer(fileSize);

	file.seekg(0);
	file.read(readBuffer.data(), fileSize);

	file.close();

	return readBuffer;
}

std::vector<const char*> getRequiredExts(const uint32_t& glfwExtCount, const char** glfwExtensions)
{
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtCount);

	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

class HelloTriangleApplication {
public:
	void run() 
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i< memoryProperties.memoryTypeCount; ++i)
		{
			if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("Failed to find valid memory type.");
	}

public:
	// Indicate if we need recreate swap chain
	bool bFrameBufferResized = false;

protected:
	// Refactor later
	void createVulkanInstance(const uint32_t& glfwExtCount, const char** glfwExtensions);
	void createSwapChain();
	void createSwapChainImageView();
	void createRenderPass();
    void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createFrameBuffers();
	void createCommandPool();
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffer();
	void createDescriptorPool();
	void createDescriptorSet();
	void createCommandBuffers();
	void createSyncObjects();

	void recreateSwapChain();
	void cleanupSwapChain();

	void createBuffer(VkDeviceMemory& bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperty, VkBuffer& buffer);
	void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkDeviceMemory& memory, VkImage& image);
	void copyBuffer2Image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout preLayout, VkImageLayout targetLayout);

	VkImageView createImageView(VkImage image, VkFormat format);

	VkShaderModule createShaderModule(const std::vector<char>& code);

	void updateUniformBuffer(uint32_t imageIdx);

	void draw();

	void createSurface()
	{
		if (glfwCreateWindowSurface(vulkanInstance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const VkQueueFamilyProperties& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport)
			{
				indices.presentFamily = i;
			}

			if (indices.IsComplete())
			{
				break;
			}

			++i;
		}

		return indices;
	}

	/// for swap chain create
	// - surface: color/ depth
	// - present: picture to screen
	// - extent: resolution in swap chain
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			return { VK_FORMAT_B8G8R8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		for (const auto& format : availableFormats)
		{
			if (format.format == VK_FORMAT_B8G8R8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return format;
			}
		}

		return availableFormats[0];
	}
	// 

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		VkPresentModeKHR selectedMode = VK_PRESENT_MODE_FIFO_KHR;
		// 
		for (const VkPresentModeKHR& presentMode : availablePresentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return presentMode;
			}
			else if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				selectedMode = presentMode;
			}
		}

		return selectedMode;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			VkExtent2D actualExtent = { WIDTH, HEIGHT };
			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.minImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.minImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}
	///


	/// Device validation
	void enumPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			throw std::runtime_error("No physical vulkan device found.");
		}

		std::vector<VkPhysicalDevice> deviceList(deviceCount);
		vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, deviceList.data());

		for (const VkPhysicalDevice& pendingDevice : deviceList)
		{
			// Select first device at the moment
			if (isDeviceSupported(pendingDevice))
			{
				physicalDevice = pendingDevice;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("No suitable physical vulkan device found.");
		}
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		// enumerate device extension list
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

		// swap chain extension check
		for (const auto& ext : availableExtensions)
		{
			requiredExtensions.erase(ext.extensionName);
		}

		return requiredExtensions.empty();
	}

	bool isDeviceSupported(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProperty;
		vkGetPhysicalDeviceProperties(device, &deviceProperty);

		VkPhysicalDeviceFeatures deviceFeature;
		vkGetPhysicalDeviceFeatures(device, &deviceFeature);

		QueueFamilyIndices indices = findQueueFamilies(device);
		bool extSupport = checkDeviceExtensionSupport(device);
		bool swapChainAdequate = false;
		if (extSupport)
		{
			SwapChainSupportDetail swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.IsComplete() && extSupport && swapChainAdequate;
	}
	/// Finish device validation

	void createLogicalDevice()
	{
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		// 
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

		float queuePriority = 1.0f;
		for (int queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		// Or use VkPhysicalDeviceFeatures2 to link other extensions, same as VkDeviceCreateInfo.pNext
		VkPhysicalDeviceFeatures deviceFeature = {};
		deviceFeature.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pEnabledFeatures = &deviceFeature;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
		createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

		// Enable uint8 as index extension
		VkPhysicalDeviceIndexTypeUint8FeaturesEXT enableUINT8Index = { 
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT, 
			nullptr, 
			VK_TRUE 
		};
		createInfo.pNext = &enableUINT8Index;

#ifdef _DEBUG
		createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
#else
		createInfo.enabledLayerCount = 0;
#endif
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create logical device.");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
	}

	SwapChainSupportDetail querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetail details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

#ifdef _DEBUG
	// SEVERITY
	// used for diagnostic 
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT£  0x0001

	// used for resources creation
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT£     0x0010

	// warning and error
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT£  0x0100
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT£    0x1000

	// MESSAGE TYPE
	// VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT       0x01
	// VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT    0x02
	// VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT   0x04

	static VKAPI_ATTR VkBool32 debugVKCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::cerr << "[DEBUG]: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	void SetupDebugInfo()
	{
		VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

		debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		debugInfo.pfnUserCallback = debugVKCallback;
		debugInfo.pUserData = nullptr;

		if (createDebugUtilsMessengerEXT(vulkanInstance, &debugInfo, nullptr, &debugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}
#endif

private:
	GLFWwindow* window;
	VkInstance vulkanInstance;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkRenderPass renderPass;
	VkCommandPool commandPool;

	VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
	
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	// For synchronrization
	size_t currentFrame = 0;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishSemaphores;
	std::vector<VkFence> presentFences;

	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	//
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffer;
	std::vector<VkDeviceMemory> uniformBufferMemory;

	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
	VkSampler defaultSampler;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

#ifdef _DEBUG
	VkDebugUtilsMessengerEXT debugMessenger;
#endif

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
		{
			auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
			if (app)
			{
				app->bFrameBufferResized = true;
			}
			else
			{
				throw std::runtime_error("Failed to cast glfw user pointer to specified type!");
			}
		});
		//glfwSetFramebufferSizeCallback(window, OnFrameBufferResized);
	}

	void initVulkan()
	{
		uint32_t glfwExtCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);

		createVulkanInstance(glfwExtCount, glfwExtensions);
#ifdef _DEBUG
		SetupDebugInfo();
#endif
		createSurface();
		enumPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createSwapChainImageView();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createFrameBuffers();
		createCommandPool();
		createTextureImage();
		createTextureImageView();
		createTextureSampler();
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffer();
		createDescriptorPool();
		createDescriptorSet();
		createCommandBuffers();
		createSyncObjects();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			draw();
		}

		//
		//vkDeviceWaitIdle(device);
	}

	void cleanup()
	{
#ifdef _DEBUG
		destroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, nullptr);
#endif
		cleanupSwapChain();

		vkDestroySampler(device, defaultSampler, nullptr);
		vkDestroyImageView(device, imageView, nullptr);

		for (size_t i = 0; i< MAX_FRAMES_IN_SWAPCHAIN; ++i)
		{
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, renderFinishSemaphores[i], nullptr);
			vkDestroyFence(device, presentFences[i], nullptr);
		}

		//
		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);

		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);

		vkDestroyImage(device, image, nullptr);
		vkFreeMemory(device, imageMemory, nullptr);

		for (auto framebuffer: swapChainFramebuffers)
		{
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		for (auto imageView: swapChainImageViews)
		{
			vkDestroyImageView(device, imageView, nullptr);
		}

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyCommandPool(device, commandPool, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroySwapchainKHR(device, swapChain, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
		vkDestroyInstance(vulkanInstance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void HelloTriangleApplication::createVulkanInstance(const uint32_t& glfwExtCount, const char** glfwExtensions)
{
#ifdef _DEBUG
	// Check validation layer
	if (!checkValidationLayerSupport())
	{
		throw std::runtime_error("validation layers required, while no available layer exists.");
	}
#endif
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = APPNAME;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = APPNAME;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
#ifdef _DEBUG
	createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
	createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

	auto extensions = getRequiredExts(glfwExtCount, glfwExtensions);

	// enabled device extension
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
	{
		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

		debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		debugInfo.pfnUserCallback = debugVKCallback;
		debugInfo.pUserData = nullptr;
	}
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugInfo;
#else
	createInfo.enabledLayerCount = 0;

	createInfo.enabledExtensionCount = glfwExtCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;
#endif

	if (vkCreateInstance(&createInfo, nullptr, &vulkanInstance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

void HelloTriangleApplication::createSwapChain()
{
	SwapChainSupportDetail swapChainDetails = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainDetails.presentModes);
	VkExtent2D swapExtent = chooseSwapExtent(swapChainDetails.capabilities);

	// for tripple buffer
	uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;
	if (swapChainDetails.capabilities.maxImageCount > 0 &&
		imageCount > swapChainDetails.capabilities.maxImageCount)
	{
		imageCount = swapChainDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;

	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = swapExtent;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.imageArrayLayers = 1; // for VR device might has more layer

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };
	if (indices.graphicsFamily != indices.presentFamily)
	{
		// VK_SHARING_MODE_CONCURRENT owned by multiple graphic queue
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
	}
	else
	{
		// VK_SHARING_MODE_EXCLUSIVE owned by one graphic queue
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
	}
	swapChainCreateInfo.preTransform = swapChainDetails.capabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swap chain.");
	}

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = swapExtent;
}

void HelloTriangleApplication::createSwapChainImageView()
{
	size_t swapChainImageCount = swapChainImages.size();
	swapChainImageViews.resize(swapChainImageCount);

	for (int i = 0; i < swapChainImageCount; ++i)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		createInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
		
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.layerCount = 1;
		createInfo.subresourceRange.levelCount = 1;

		if (!vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) == VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create swap chain image view.");
		}
	}
}

void HelloTriangleApplication::createRenderPass()
{
	// Bind to layout in VS
	VkAttachmentDescription attachmentColor = {};
	attachmentColor.format = swapChainImageFormat;
	attachmentColor.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentColor.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // clear with constants
	attachmentColor.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentColor.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentColor.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentColor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentColor.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference attachmentColorRef;
	attachmentColorRef.attachment = 0;
	attachmentColorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentColorRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0; // index, not null
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &attachmentColor;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass..");
	}
}

void HelloTriangleApplication::createDescriptorSetLayout()
{
	// uniform buffer object
	VkDescriptorSetLayoutBinding uboBinding = {};
	uboBinding.binding = 0;
	uboBinding.descriptorCount = 1;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboBinding.pImmutableSamplers = nullptr;

	// image sampler
	VkDescriptorSetLayoutBinding samplerBinding = {};
	samplerBinding.binding = 1;
	samplerBinding.descriptorCount = 1;
	samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerBinding.pImmutableSamplers = nullptr;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboBinding, samplerBinding };

	VkDescriptorSetLayoutCreateInfo descSetLayoutInfo;
	ZeroVkStructure(descSetLayoutInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
	descSetLayoutInfo.bindingCount = bindings.size();
	descSetLayoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &descSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout..");
	}
}

void HelloTriangleApplication::createGraphicsPipeline()
{
	auto vsCode = ReadFile("../Shader/vert.spv");
	auto fsCode = ReadFile("../Shader/frag.spv");

	VkShaderModule vsModule = createShaderModule(vsCode);
	VkShaderModule fsModule = createShaderModule(fsCode);

	// create graphic pipeline
	VkPipelineShaderStageCreateInfo vsShaderStageCreateInfo = {};
	vsShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vsShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vsShaderStageCreateInfo.module = vsModule;
	vsShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fsShaderStageCreateInfo = {};
	fsShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fsShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fsShaderStageCreateInfo.module = fsModule;
	fsShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vsShaderStageCreateInfo, fsShaderStageCreateInfo };

	// Create vertex input layout
	VkVertexInputBindingDescription vertexBindingDesc = Vertex::getDescription();
	std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDesc = Vertex::getAttributeDescription();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDesc;
	vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributeDesc.size();
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDesc.data();

	// Setup Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	// Setup viewport
	VkViewport viewport = { 0, 0, (float)swapChainExtent.width, (float)swapChainExtent.height, 0.0f, 1.0f };
	VkRect2D scissor = { VkOffset2D{0, 0}, swapChainExtent };
	VkPipelineViewportStateCreateInfo viewportStateInfo = {};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.pViewports = &viewport;
	viewportStateInfo.pScissors = &scissor;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.scissorCount = 1;

	// Rasterization
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// MSAA - disable it for now
	VkPipelineMultisampleStateCreateInfo multiSample = {};
	multiSample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSample.sampleShadingEnable = VK_FALSE;
	multiSample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multiSample.minSampleShading = 1.0f;
	multiSample.pSampleMask = nullptr;
	multiSample.alphaToCoverageEnable = VK_FALSE;
	multiSample.alphaToOneEnable = VK_FALSE;

	// Depth and Stencil

	// Color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {}; // for single frame buffer
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlend = {}; // global state
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.logicOpEnable = VK_FALSE;
	colorBlend.logicOp = VK_LOGIC_OP_COPY;
	colorBlend.attachmentCount = 1;
	colorBlend.pAttachments = &colorBlendAttachment;
	colorBlend.blendConstants[0] = 0.0f;
	colorBlend.blendConstants[1] = 0.0f;
	colorBlend.blendConstants[2] = 0.0f;
	colorBlend.blendConstants[3] = 0.0f;

	// Setup dynamic state
	VkDynamicState dynamicState[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = 2;
	dynamicStateInfo.pDynamicStates = dynamicState;

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multiSample;
	pipelineInfo.pColorBlendState = &colorBlend;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline..!");
	}

	vkDestroyShaderModule(device, fsModule, nullptr);
	vkDestroyShaderModule(device, vsModule, nullptr);
}

void HelloTriangleApplication::createFrameBuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i< swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = { swapChainImageViews [i]};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create buffer..");
		}
	}
}

void HelloTriangleApplication::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo commandPoolInfo = {};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	//commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool..");
	}
}

void HelloTriangleApplication::createTextureImage()
{
	int width, height, channels;
	stbi_uc* pixels = stbi_load("../Texture/placeholder.jpg", &width, &height, &channels, STBI_rgb_alpha);

	VkDeviceSize imageSize = width * height * 4;

	if (!pixels)
	{
		throw std::runtime_error("Failed to load external texture.");
	}

	// Create buffer for texture
	VkBuffer stageBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(stagingBufferMemory,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stageBuffer);

	// Copy texture data to stage buffer 
	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy_s(data, imageSize, pixels, imageSize);
	vkUnmapMemory(device, stagingBufferMemory);

	stbi_image_free(pixels);
	
	createImage(
		width, 
		height, 
		VK_FORMAT_R8G8B8A8_UNORM, 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		imageMemory,
		image);

	transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBuffer2Image(stageBuffer, image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(device, stageBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void HelloTriangleApplication::createTextureImageView()
{
	imageView = createImageView(image, VK_FORMAT_R8G8B8A8_UNORM);



}

void HelloTriangleApplication::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo;
	ZeroVkStructure(samplerInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0;
	samplerInfo.minLod = 0; // float why?
	samplerInfo.maxLod = 0;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create default sampler");
	}
}

void HelloTriangleApplication::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(DummyVertices[0]) * DummyVertices.size();
	
	// Traditional method to use one buffer to transfer from cpu to gpu
	//createBuffer(vertexBufferMemory, 
	//			 bufferSize,
	//	         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	//	         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	//	         vertexBuffer);

	//// map dummy vertices mem to gpu
	//void* data = nullptr;
	//vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);
	//	memcpy(data, DummyVertices.data(), static_cast<size_t>(bufferSize));
	//vkUnmapMemory(device, vertexBufferMemory);

	// Transfer specified buffer to GPU
	VkBuffer stageBuffer;
	VkDeviceMemory stageBufferMemory;
	void* data = nullptr;
	
	createBuffer(stageBufferMemory, 
				 bufferSize,
		         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		         stageBuffer);

	// Map vertex buffer data to stage buffer memory
	vkMapMemory(device, stageBufferMemory, 0, bufferSize, 0, &data);
		memcpy_s(data, bufferSize, DummyVertices.data(), bufferSize);
	vkUnmapMemory(device, stageBufferMemory);

	// Use GPU local memory, it can get better perf
	createBuffer(vertexBufferMemory,
		         bufferSize,
		         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		         vertexBuffer);

	copyBuffer(stageBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(device, stageBuffer, nullptr);
	vkFreeMemory(device, stageBufferMemory, nullptr);
}

void HelloTriangleApplication::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(DummyIndices[0]) * DummyIndices.size();

	// Transfer specified buffer to GPU
	VkBuffer stageBuffer;
	VkDeviceMemory stageBufferMemory;
	void* data = nullptr;

	createBuffer(stageBufferMemory,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stageBuffer);

	// Map index buffer data to stage buffer memory
	vkMapMemory(device, stageBufferMemory, 0, bufferSize, 0, &data);
		memcpy_s(data, bufferSize, DummyIndices.data(), bufferSize);
	vkUnmapMemory(device, stageBufferMemory);

	// Use GPU local memory, it can get better perf
	createBuffer(indexBufferMemory,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer);

	copyBuffer(stageBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(device, stageBuffer, nullptr);
	vkFreeMemory(device, stageBufferMemory, nullptr);
}

void HelloTriangleApplication::createUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(UniformBuffer);
	uniformBuffer.resize(swapChainImages.size());
	uniformBufferMemory.resize(swapChainImages.size());

	for (size_t i = 0; i< swapChainImages.size(); ++i)
	{
		createBuffer(uniformBufferMemory[i], 
			bufferSize, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 
			uniformBuffer[i]);
	}
}

void HelloTriangleApplication::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSize;
	//
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

	VkDescriptorPoolCreateInfo descPoolInfo;
	ZeroVkStructure(descPoolInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
	descPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
	descPoolInfo.pPoolSizes = poolSize.data(); 
	descPoolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

	if (vkCreateDescriptorPool(device, &descPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool..");
	}
}

void HelloTriangleApplication::createDescriptorSet()
{
	std::vector<VkDescriptorSetLayout> descSetLayouts(swapChainImages.size(), descriptorSetLayout);
	
	VkDescriptorSetAllocateInfo descSetAllocInfo;
	ZeroVkStructure(descSetAllocInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
	descSetAllocInfo.descriptorPool = descriptorPool;
	descSetAllocInfo.descriptorSetCount = swapChainImages.size();
	descSetAllocInfo.pSetLayouts = descSetLayouts.data();

	// descriptorSets will be free after descriptor pool destroy
	descriptorSets.resize(swapChainImages.size());
	if (vkAllocateDescriptorSets(device, &descSetAllocInfo, descriptorSets.data()) != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to allocate descriptor sets..");
	}

	// create descriptor buffer
	for (size_t i = 0; i < swapChainImages.size(); i++) 
	{
		VkDescriptorBufferInfo descBufferInfo = {};
		descBufferInfo.buffer = uniformBuffer[i];
		descBufferInfo.offset = 0;
		descBufferInfo.range = sizeof(UniformBuffer);

		VkDescriptorImageInfo descImageInfo = {};
		descImageInfo.sampler = defaultSampler;
		descImageInfo.imageView = imageView;
		descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		// Uniform buffer
		ZeroVkStructure(descriptorWrites[0], VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0; // index
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &descBufferInfo;

		// Sampler
		ZeroVkStructure(descriptorWrites[1], VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1; // binding index
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &descImageInfo;

		// Multiple descriptor need update
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void HelloTriangleApplication::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {}; 
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO; 
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffer..");
	}

	// Record commands to command buffer
	for (size_t i = 0; i< commandBuffers.size(); ++i)
	{
		VkCommandBufferBeginInfo cmdBeginInfo = {};
		cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		cmdBeginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffers[i], &cmdBeginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command buffers..");
		}

		// Rendering process will begin once vkCmdBeginRenderPass invoked
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[i];
		renderPassInfo.renderArea.extent = swapChainExtent;
		renderPassInfo.renderArea.offset = { 0, 0 };
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };  // For VK_ATTACHMENT_LOAD_OP_CLEAR
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		// Begin render pass
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			VkBuffer vertexBuffers[] = { vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
			//// uint8 need extra extension to support, check if VkPhysicalDeviceIndexTypeUint8FeaturesEXT enabled.
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT8_EXT);
			//vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

			//vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(DummyVertices.size()), 1, 0, 0);
			vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(DummyIndices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer..");
		}
	}
}

void HelloTriangleApplication::createSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_SWAPCHAIN);
	renderFinishSemaphores.resize(MAX_FRAMES_IN_SWAPCHAIN);
	presentFences.resize(MAX_FRAMES_IN_SWAPCHAIN);

	VkSemaphoreCreateInfo semaphoreInfo = {}; 
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i= 0; i< MAX_FRAMES_IN_SWAPCHAIN; ++i)
	{
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, & fenceInfo, nullptr, &presentFences[i]))
		{
			throw std::runtime_error("Failed to create semaphore");
		}
	}
}

void HelloTriangleApplication::recreateSwapChain()
{
	// minimize
	int width = 0;
	int height = 0;
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	cleanupSwapChain();

	createSwapChain();
	createSwapChainImageView();
	createRenderPass();
	createGraphicsPipeline();
	createFrameBuffers();
	createUniformBuffer();
	createDescriptorPool();
	createDescriptorSet();
	createCommandBuffers();
}

void HelloTriangleApplication::cleanupSwapChain()
{
	// Cleanup frame buffer, command buffer, pipeline object, pipeline layout, render pass, image view, swap chain in order
	for (size_t i = 0; i< swapChainFramebuffers.size(); ++i)
	{
		vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
	}

	vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());

	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);

	for (size_t i = 0; i < swapChainImages.size(); i++) 
	{
		vkDestroyBuffer(device, uniformBuffer[i], nullptr);
		vkFreeMemory(device, uniformBufferMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);

	for (size_t i = 0; i < swapChainImageViews.size(); ++i)
	{
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void HelloTriangleApplication::createBuffer(VkDeviceMemory& bufferMemory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperty, VkBuffer& buffer)
{
	VkBufferCreateInfo bufferInfo;
	ZeroVkStructure(bufferInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // For now

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create buffer.");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo;
	ZeroVkStructure(allocInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, memoryProperty);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate buffer memory.");
	}

	//
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void HelloTriangleApplication::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	VkCommandBuffer cmdBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;

		vkCmdCopyBuffer(cmdBuffer, src, dst, 1, &copyRegion);
		// vkCmdCopyAccelerationStructureNV() // Acceleration Structure?
	
	endSingleTimeCommands(cmdBuffer);
}

void HelloTriangleApplication::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkDeviceMemory& memory, VkImage& image)
{
	VkImageCreateInfo imageInfo;
	ZeroVkStructure(imageInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image.");
	}

	// Allocate image memory
	VkMemoryRequirements imageMemRequirements;
	vkGetImageMemoryRequirements(device, image, &imageMemRequirements);

	VkMemoryAllocateInfo allocInfo;
	ZeroVkStructure(allocInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
	allocInfo.allocationSize = imageMemRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(imageMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate image memory");
	}

	vkBindImageMemory(device, image, memory, 0);
}

void HelloTriangleApplication::copyBuffer2Image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer cmdBuffer = beginSingleTimeCommands();

	VkBufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferImageHeight = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageExtent = { width, height, 1 };
	copyRegion.imageOffset = { 0, 0 };

	vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	endSingleTimeCommands(cmdBuffer);
}

VkCommandBuffer HelloTriangleApplication::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo cmdAllocInfo;
	ZeroVkStructure(cmdAllocInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdAllocInfo.commandPool = commandPool;
	cmdAllocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &cmdAllocInfo, &commandBuffer);

	VkCommandBufferBeginInfo cmdBeginInfo;
	ZeroVkStructure(cmdBeginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo);

	return commandBuffer;
}

void HelloTriangleApplication::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo;
	ZeroVkStructure(submitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO);
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void HelloTriangleApplication::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout preLayout, VkImageLayout targetLayout)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier;
	ZeroVkStructure(barrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
	barrier.oldLayout = preLayout;
	barrier.newLayout = targetLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	//
	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	if (preLayout == VK_IMAGE_LAYOUT_UNDEFINED && targetLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (preLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && targetLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::invalid_argument("Incorrect layout transition.");
	}


	vkCmdPipelineBarrier(commandBuffer,
		srcStage, // src stage mask
		dstStage, // dst stage mask
		0, // dependency flag
		0, // memory barrier count
		nullptr, // memory barrier
		0, // buffer memory barrier count
		nullptr, // buffer memory barrier
		1, // image memory barrier count
		&barrier // image memory barrier
		);

	endSingleTimeCommands(commandBuffer);
}

VkImageView HelloTriangleApplication::createImageView(VkImage image, VkFormat format)
{
	VkImageView view;

	VkImageViewCreateInfo imageViewInfo;
	ZeroVkStructure(imageViewInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
	imageViewInfo.image = image;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.format = format;
	imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	imageViewInfo.subresourceRange.layerCount = 1;
	imageViewInfo.subresourceRange.levelCount = 1;

	if (vkCreateImageView(device, &imageViewInfo, nullptr, &view) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view.");
	}

	return view;
}

VkShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule))
	{
		throw std::runtime_error("Failed to create shader module..");
	}

	return shaderModule;
}

void HelloTriangleApplication::updateUniformBuffer(uint32_t imageIdx)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float duration = std::chrono::duration<float, std::chrono::seconds::period>(currentTime- startTime).count();

	UniformBuffer ubo = {};
	ubo.model = glm::rotate(Matrix4(1.0f), duration* glm::radians(90.0f), Vector3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(Vector3(2.0f, 2.0f, 2.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f));
	ubo.projection = glm::perspective(glm::radians(45.0f), swapChainExtent.width/ (float)swapChainExtent.height, 0.01f, 100.0f);
	ubo.projection[1][1] *= -1.0f;	// Matrix should be row major in VK

	// Map vertex buffer data to stage buffer memory
	void* data = nullptr;
	size_t bufferSize = sizeof(UniformBuffer);
	vkMapMemory(device, uniformBufferMemory[imageIdx], 0, bufferSize, 0, &data);
		memcpy_s(data, bufferSize, &ubo, bufferSize);
	vkUnmapMemory(device, uniformBufferMemory[imageIdx]);
}

void HelloTriangleApplication::draw()
{
	vkWaitForFences(device, 1, &presentFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t >::max());

	// Get image from swap chain
	uint32_t imageIdx = 0;
	constexpr uint64_t timeOut = std::numeric_limits<uint64_t >::max();

	// If we got available image, acquire it otherwise block it.
	VkResult ret = vkAcquireNextImageKHR(device, swapChain, timeOut, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIdx);
	if (ret == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}
	else if (ret != VK_SUCCESS && ret != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image.");
	}

	// Prepare submit to command list
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphore = imageAvailableSemaphores[currentFrame];
	VkPipelineStageFlags waitStage[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &waitSemaphore;
	submitInfo.pWaitDstStageMask = waitStage;

	// Specify which command buffer to submit
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIdx];

	VkSemaphore signalSemaphore = renderFinishSemaphores[currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &signalSemaphore;

	vkResetFences(device, 1, &presentFences[currentFrame]);

	// Updated should be ahead of commands submit
	updateUniformBuffer(imageIdx);

	// Submit to graphic command queue
	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, presentFences[currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit to graphic command queue..");
	}

	//
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &signalSemaphore;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIdx;
	presentInfo.pResults = nullptr;

	ret = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (ret == VK_ERROR_OUT_OF_DATE_KHR || ret == VK_SUBOPTIMAL_KHR || bFrameBufferResized)
	{
		bFrameBufferResized = false;
		recreateSwapChain();
	}
	else if (ret != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swap chain image.");
	}

	currentFrame = (currentFrame+ 1)% MAX_FRAMES_IN_SWAPCHAIN;
}

