#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <optional>

#include "window.h"

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;

	bool is_complete() {
		return graphicsFamily.has_value();
	}
};

QueueFamilyIndices find_queue_families(VkPhysicalDevice device) {
	QueueFamilyIndices indices;
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	int i = 0;
	for (const auto &queue_family : queue_families) {
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		if (indices.is_complete()) break;

		i++;
	}

	return indices;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

class Application {
public:
	static constexpr int WIDTH = 800;
	static constexpr int HEIGHT = 600;
	Application(void) {}
	~Application(void)
	{
		vkDestroyDevice(device, nullptr);
		if (enable_validation_layers) {
			DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
		}
		vkDestroyInstance(instance, nullptr);
	}
	void run()
	{
		init_vulkan();
		main_loop();
	}
private:
	// Windowing / Instance
	Window window{ WIDTH, HEIGHT, "Vulkan" };
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	std::string name;

	// Physical Device
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	
	// Logical Device
	VkDevice device;
	VkQueue graphics_queue;

	const std::vector<const char *> validation_layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	#ifdef NDEBUG
		const bool enable_validation_layers = false;
	#else
		const bool enable_validation_layers = true;
	#endif
private:
	void init_vulkan()
	{
		create_instance();
		setup_debug_messenger();
		pick_physical_device();
		create_logical_device();
	}
	void main_loop()
	{
		while (!window.should_close())
		{
			glfwPollEvents();
		}
	}

	void create_logical_device() {
		// Find all info needed for device creation

		// Queues
		QueueFamilyIndices indices = find_queue_families(physical_device);

		VkDeviceQueueCreateInfo queue_create_info{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = indices.graphicsFamily.value();
		queue_create_info.queueCount = 1;

		float queue_priority = 1.0f;
		queue_create_info.pQueuePriorities = &queue_priority;

		// Enabled Features
		VkPhysicalDeviceFeatures device_features{};

		// Create Device
		VkDeviceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.pQueueCreateInfos = &queue_create_info;
		create_info.queueCreateInfoCount = 1;

		create_info.pEnabledFeatures = &device_features;

		// Legacy Compatability
		create_info.enabledExtensionCount = 0;

		if (enable_validation_layers) {
			create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
			create_info.ppEnabledLayerNames = validation_layers.data();
		}
		else {
			create_info.enabledLayerCount = 0;
		}

		// Instantiate Device
		if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create logical device!");
		}

		// Get Device Queues
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphics_queue);
	}

	void pick_physical_device() {
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
		if (device_count == 0) {
			throw std::runtime_error("Failed to find GPUs with Vulkan Support!");
		}

		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
		for (const auto &device : devices) {
			if (is_device_suitable(device)) {
				physical_device = device;
				break;
			}
		}

		if (physical_device == VK_NULL_HANDLE) {
			throw std::runtime_error("Failed to find a suitable GPU!");
		}
	}

	bool is_device_suitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = find_queue_families(device);
		return indices.is_complete();
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	void setup_debug_messenger() {
		if (!enable_validation_layers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debug_messenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	bool check_validation_layers_support() {
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector < VkLayerProperties > available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
		for (const char * layer_name : validation_layers) {
			bool found_layer = false;
			for (const auto & layer_properties : available_layers) {
				if (strcmp(layer_name, layer_properties.layerName) == 0) {
					found_layer = true;
					break;
				}
			}

			if (!found_layer) return false;
		}
		return true;
	}

	std::vector<const char *> get_required_extensions() {
		uint32_t glfw_extension_count = 0;
		const char ** glfw_extensions;
		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

		if (enable_validation_layers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	void create_instance()
	{
		if (enable_validation_layers && !check_validation_layers_support()) {
			throw std::runtime_error("Validation layers requested, but not available!");
		}

		// Application Info
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulkan Renderer";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// Validation and Extensions
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// GLFW Extensions
		uint32_t glfwExtensionCount = 0;
		const char ** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;

		// Validation Layer and Extensions
		if (enable_validation_layers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
			createInfo.ppEnabledLayerNames = validation_layers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}
		auto extensions = get_required_extensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// Debug

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (enable_validation_layers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
			createInfo.ppEnabledLayerNames = validation_layers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}


		// Result
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create instance!");
		}
	}
};