#include "VEbase.h"

std::vector<const char*> validationLayers{
	"VK_LAYER_KHRONOS_validation",
};

std::vector<const char*> instanceExtensions{
	VK_KHR_SURFACE_EXTENSION_NAME,
};

std::vector<const char*> deviceExtensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

void VEbase::init() {
	setUpWindow(title);
	glfwSetKeyCallback(window, key_callback);

	updateInstanceExtensions();

	createInstance();
	setUpDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createSwapChainImageViews();
}

void VEbase::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		keyHandle();

		drawFrame();
	}

	device.waitIdle();
}

void VEbase::cleanUpBase() {
	for (int i = 0; i < swapChainImageViews.size(); i++) {
		device.destroyImageView(swapChainImageViews[i]);
	}

	device.destroySwapchainKHR(swapChain);
	device.destroy();

	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	instance.destroySurfaceKHR(surface);
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
		.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
		.ppEnabledExtensionNames = instanceExtensions.data(),
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
		if (isDeviceSuitable(candidate, surface)) {
			physicalDevice = candidate;
		}
	}
}

void VEbase::createLogicalDevice() {
	auto indices{ findQueueFamilies(physicalDevice, surface) };

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float priority = 1.0f;

	for (uint32_t queueFamily : uniqueQueueFamilies) {
		vk::DeviceQueueCreateInfo queueInfo{
			.queueFamilyIndex = queueFamily,
			.queueCount = 1,
			.pQueuePriorities = &priority,
		};

		queueCreateInfos.push_back(queueInfo);
	}

	vk::PhysicalDeviceFeatures features{};
	vk::DeviceCreateInfo deviceInfo{
		.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		// geometry Shader, tessellationShader, samplerAnistrophy etc...
		.pEnabledFeatures = &features,
	};

	device = physicalDevice.createDevice(deviceInfo);
	device.getQueue(indices.graphicsFamily.value(), 0, &graphicsQueue);
	device.getQueue(indices.presentFamily.value(), 0, &presentQueue);
}

void VEbase::createSurface() {
	VkWin32SurfaceCreateInfoKHR surfaceInfo{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = GetModuleHandle(nullptr),
		.hwnd = glfwGetWin32Window(window),
	};

	VK_CHECK_RESULT(vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, &surface));
}

void VEbase::createSwapChain() {
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

	vk::SurfaceFormatKHR surfaceFormat = chooseSwapChainSurfaceFormat(swapChainSupport.formats);
	vk::PresentModeKHR presentMode = chooseSwapChainPresentMode(swapChainSupport.presentModes);
	vk::Extent2D extent = chooseSwapChainExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR swapChainInfo{
		.surface = surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
		.presentMode = presentMode,
	};

	auto indices{ findQueueFamilies(physicalDevice, surface) };
	uint32_t queueFamilyIndices[]{ indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		swapChainInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapChainInfo.queueFamilyIndexCount = 2;
		swapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		swapChainInfo.imageSharingMode = vk::SharingMode::eExclusive;
	}

	swapChainInfo.preTransform = swapChainSupport.capabilities.currentTransform;	// rotation or flip of the view
	swapChainInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

	swapChainInfo.presentMode = presentMode;
	swapChainInfo.clipped = vk::True;

	swapChain = device.createSwapchainKHR(swapChainInfo);

	auto images = device.getSwapchainImagesKHR(swapChain);
	
	swapChainImages = std::vector<vk::Image>(images.begin(), images.end());

	swapChainFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void VEbase::createSwapChainImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());

	for (int i = 0; i < swapChainImages.size(); i++) {
		vk::ImageViewCreateInfo imageViewInfo{
			.image = swapChainImages[i],
			.viewType = vk::ImageViewType::e2D,
			.format = swapChainFormat,
			.components = {
				.r = vk::ComponentSwizzle::eR,
				.g = vk::ComponentSwizzle::eG,
				.b = vk::ComponentSwizzle::eB,
				.a = vk::ComponentSwizzle::eA,
			},
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		swapChainImageViews[i] = device.createImageView(imageViewInfo);
	}
}

void VEbase::recreateSwapChain()
{
	int width{}, height{};
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	device.waitIdle();

	destroyFrameBuffers();

	for (auto i = 0; i < swapChainImageViews.size(); i++) {
		device.destroyImageView(swapChainImageViews[i]);
	}

	device.destroySwapchainKHR(swapChain);

	createSwapChain();
	createSwapChainImageViews();
	createFrameBuffers();
}

vk::SurfaceFormatKHR VEbase::chooseSwapChainSurfaceFormat(std::vector<vk::SurfaceFormatKHR> availableFormats) {
	for (auto format : availableFormats) {
		if (format.format == vk::Format::eB8G8R8A8Srgb &&
			format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return format;
		}
	}
	
	return availableFormats[0];
}

vk::PresentModeKHR VEbase::chooseSwapChainPresentMode(std::vector<vk::PresentModeKHR> availableModes) {
	for (auto mode : availableModes) {
		if (mode == vk::PresentModeKHR::eMailbox) {
			return mode;
		}
	}

	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VEbase::chooseSwapChainExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		vk::Extent2D actualExtent{
			.width = static_cast<uint32_t>(width),
			.height = static_cast<uint32_t>(height),
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

vk::ShaderModule VEbase::createShaderModule(std::vector<char>& code)
{
	vk::ShaderModuleCreateInfo shaderModuleInfo{
		.codeSize = code.size(),
		.pCode = reinterpret_cast<const uint32_t*>(code.data()),
	};

	auto module = device.createShaderModule(shaderModuleInfo);

	return module;
}

bool isDeviceSuitable(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
	auto indice = findQueueFamilies(device, surface);

	bool extensionSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indice.isComplete() && extensionSupported && swapChainAdequate;
}

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
	auto queueFamilies{ device.getQueueFamilyProperties() };

	QueueFamilyIndices indices{};

	int i{};
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = i;
		}

		// present support
		if (device.getSurfaceSupportKHR(i, surface)) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

bool checkDeviceExtensionSupport(vk::PhysicalDevice device) {
	auto availbleExtensions = device.enumerateDeviceExtensionProperties();

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availbleExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
	SwapChainSupportDetails details{
		.capabilities = device.getSurfaceCapabilitiesKHR(surface),
		.formats = device.getSurfaceFormatsKHR(surface),
		.presentModes = device.getSurfacePresentModesKHR(surface),
	};

	return details;
}

std::vector<char> readFileAsBinary(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		std::cout << "file not opened\n";
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

const std::string getShadersPath()
{
#if defined(SHADERS_DIR)
	return SHADERS_DIR;
#endif
	return "shaders/";
};

void updateInstanceExtensions() {
	uint32_t count;
	auto glExtensions = glfwGetRequiredInstanceExtensions(&count);
	for (uint32_t i{}; i < count; i++) {
		bool flag = false;
		for (auto item : instanceExtensions) {
			if (strcmp(item, glExtensions[i]) == 0) {
				flag = true;
			}
		}

		if (!flag) {
			instanceExtensions.push_back(glExtensions[i]);
		}
	}

	if (enableValidationLayers) {
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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

	std::cerr << "validation layer: " << pCallbackData->pMessage << "\n\n";

	return VK_FALSE;
}

// ------------------- Window -----------------------

std::queue<int> KeyEventQueue{};

void VEwindow::setUpWindow(const char* title, int width, int height) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	VEwindow::window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

void VEwindow::cleanUpWindow() {
	glfwDestroyWindow(VEwindow::window);
	glfwTerminate();
}

void VEwindow::setWindowTitle(std::string title) {
	glfwSetWindowTitle(VEwindow::window, title.c_str());
}

void VEwindow::keyProcess(GLFWwindow* win, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_UNKNOWN) return; // Don't accept unknown keys

	if (action == GLFW_PRESS) {
		pressed[key] = true;
		KeyEventQueue.push(key);
	}
	else if (action == GLFW_RELEASE)
		pressed[key] = false;

	switch (key) {
	case GLFW_KEY_ESCAPE:
		if (action == GLFW_PRESS)
			glfwSetWindowShouldClose(win, true);
	}
}

// 메인루프에서 호출하여 키 눌렸는지 확인하는 용도
void VEwindow::keyHandle() {
	for (int i = 0; i < KEYS; i++) {
		if (!pressed[i]) continue;
		switch (i) {
		case GLFW_KEY_P:
			exit(-1);
		default:
			break;
		}
	}
}

void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods) {
	auto input = glfwGetWindowUserPointer(win);
	VEwindow::keyProcess(win, key, scancode, action, mods);
}
