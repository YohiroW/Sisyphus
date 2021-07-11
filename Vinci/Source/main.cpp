#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char* APPNAME = "VINCI";

#ifdef _DEBUG
/// Validation Layer should be abstracted, but leave it here for learning usage
/// We can learn how to make validation configuration by vk_layer_settings.txt
/// Currently use default setting in this solution. 
 
const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

struct QueueFamilyIndices
{
	int graphicsFamily = -1;

	bool IsComplete()
	{
		return graphicsFamily > 0;
	}
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
	void createVulkanInstance(const uint32_t& glfwExtCount, const char** glfwExtensions);

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
		VkPhysicalDevice phyDevice = VK_NULL_HANDLE;
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
				phyDevice = pendingDevice;
				break;
			}			
		}

		if (phyDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("No vulkan device found.");
		}
	}

	bool isDeviceSupported(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProperty;
		vkGetPhysicalDeviceProperties(device, &deviceProperty);

		VkPhysicalDeviceFeatures deviceFeature;
		vkGetPhysicalDeviceFeatures(device, &deviceFeature);

		QueueFamilyIndices indices = findQueueFamilies(device);

		return deviceProperty.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	}

	/// Finish device validation



#ifdef _DEBUG
	// SEVERITY
	// used for diagnostic 
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT£º  0x0001

	// used for resources creation
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT£º     0x0010

	// warning and error
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT£º  0x0100
	// VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT£º    0x1000

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
		enumPhysicalDevice();
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
