#include "VEbase.h"

#include <vector>

std::vector<const char*> validationLayers{
	"VK_LAYER_KHRONOS_validation",
};

std::vector<const char*> InstanceExtensions{
	VK_KHR_SURFACE_EXTENSION_NAME,
};

void VEbase::init() {
	setUpWindow(title);
	glfwSetKeyCallback(window, key_callback);

	updateInstanceExtensions();

	createInstance();
	setUpDebugMessenger();
	pickPhysicalDevice();
	createLogicalDevice();
}

void VEbase::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		keyHandle();
	}
}

void VEbase::cleanUp() {
	device.destroy();

	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	instance.destroy();

	cleanUpWindow();
}

void VEbase::createInstance() {
	vk::ApplicationInfo appInfo{
		.pApplicationName = title,
		.apiVersion = vk::makeApiVersion(0, 1, 0, 0),
	};

	vk::InstanceCreateInfo instanceInfo{
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = static_cast<uint32_t>(InstanceExtensions.size()),
		.ppEnabledExtensionNames = InstanceExtensions.data(),
	};

	if (enableValidationLayers && checkValidationLayerSupport()) {
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceInfo.ppEnabledLayerNames = validationLayers.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		populateDebugMessengerCreateInfo(debugCreateInfo);
		instanceInfo.pNext = &debugCreateInfo;
	}
	else {
		instanceInfo.enabledLayerCount = 0;
		instanceInfo.ppEnabledLayerNames = nullptr;
	}

	instance = vk::createInstance(instanceInfo);
}

void VEbase::pickPhysicalDevice() {
	auto candidates{ instance.enumeratePhysicalDevices() };
	
	for (auto candidate : candidates) {
		if (isDeviceSuitable(candidate)) {
			physicalDevice = candidate;
		}
	}
}

void VEbase::createLogicalDevice() {
	auto indices{ findQueueFamilies(physicalDevice) };

	float priority = 1.0f;
	vk::DeviceQueueCreateInfo queueInfo{
		.queueFamilyIndex = indices.graphicsFamily.value(),
		.queueCount = 1,
		.pQueuePriorities = &priority,
	};

	vk::PhysicalDeviceFeatures features{};
	vk::DeviceCreateInfo deviceInfo{
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueInfo,
		// geometry Shader, tessellationShader, samplerAnistrophy etc...
		.pEnabledFeatures = &features,
	};

	device = physicalDevice.createDevice(deviceInfo);
	device.getQueue(indices.graphicsFamily.value(), 0, &graphicsQueue);
}

bool isDeviceSuitable(vk::PhysicalDevice device) {
	auto indice = findQueueFamilies(device);

	return indice.isComplete();
}

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device) {
	auto queueFamilies{ device.getQueueFamilyProperties() };

	QueueFamilyIndices indices{};

	int i{};
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

void updateInstanceExtensions() {
	uint32_t count;
	auto glExtensions = glfwGetRequiredInstanceExtensions(&count);
	for (uint32_t i{}; i < count; i++) {
		bool flag = false;
		for (auto item : InstanceExtensions) {
			if (strcmp(item, glExtensions[i]) == 0) {
				flag = true;
			}
		}

		if (!flag) {
			InstanceExtensions.push_back(glExtensions[i]);
		}
	}

	if (enableValidationLayers) {
		InstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
}

bool checkValidationLayerSupport() {
	auto availableLayers{ vk::enumerateInstanceLayerProperties() };

	for (auto layerName : validationLayers) {
		bool layerFound{ false };

		for (auto layerProperty : availableLayers) {
			if (strcmp(layerProperty.layerName, layerName)) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
			return false;
	}

	return true;
}

// ------------ Validation layer : Debug Messenger ----------------

void VEbase::setUpDebugMessenger() {
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT((VkInstance)instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType =
		//VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
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

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}
