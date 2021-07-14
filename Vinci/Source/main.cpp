#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <set>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char* APPNAME = "VINCI";

#ifdef _DEBUG
/// Validation Layer should be abstracted, but leave it here for learning usage
/// We can learn how to make validation configuration by vk_layer_settings.txt
/// Currently use default setting in this solution. 
 
const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool IsComplete()
	{
		return graphicsFamily > 0 && presentFamily > 0;
	}
};

struct SwapChainSupportDetail
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

bool checkValidationLayerSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> layerAvailable(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layerAvailable.data());

	for (const char* layer: VALIDATION_LAYERS)
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

std::vector<const char*> getRequiredExts(const uint32_t& glfwExtCount, const char** glfwExtensions)
{
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions+ glfwExtCount);

	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

protected:
	// Refactor later
	void createVulkanInstance(const uint32_t& glfwExtCount, const char** glfwExtensions);
	
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
		for (const VkQueueFamilyProperties& queueFamily: queueFamilies)
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			
			if (presentSupport)
			{
				indices.presentFamily = i;
			}

			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			if (indices.IsComplete())
			{
				break;
			}

			++i;
		}

		return indices;
	}

	/// Device validation
	void enumPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		
		vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			throw std::runtime_error("No vulkan device found.");
		}

		std::vector<VkPhysicalDevice> deviceList(deviceCount);
		vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, deviceList.data());

		for (const VkPhysicalDevice& pendingDevice: deviceList)
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
			throw std::runtime_error("No vulkan device found.");
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
		for (const auto& ext: availableExtensions)
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

		VkPhysicalDeviceFeatures deviceFeature = {};

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pEnabledFeatures = &deviceFeature;
		createInfo.enabledExtensionCount = 0;

		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = queueCreateInfos.size();

#ifdef _DEBUG
		createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
#else
		createInfo.enabledLayerCount = 0��
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
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT��  0x0001

	// used for resources creation
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT��     0x0010

	// warning and error
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT��  0x0100
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT��    0x1000

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

	VkPhysicalDevice physicalDevice { VK_NULL_HANDLE };
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

#ifdef _DEBUG
	VkDebugUtilsMessengerEXT debugMessenger;
#endif

	void initWindow() 
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void initVulkan() 
	{
		uint32_t glfwExtCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);

		createVulkanInstance(glfwExtCount, glfwExtensions);
		createSurface();
		enumPhysicalDevice();
		createLogicalDevice();
#ifdef _DEBUG
		SetupDebugInfo();
#endif
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}

	void cleanup() 
	{
#ifdef _DEBUG
		destroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, nullptr);
#endif
		vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
		vkDestroyDevice(device, nullptr);
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
	createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
	createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

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
