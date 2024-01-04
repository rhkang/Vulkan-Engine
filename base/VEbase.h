#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#undef max	// to enable numeric_limits<>::max()

#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <optional>
#include <limits>
#include <algorithm>
#include <fstream>
#include <chrono>

// ------------- Window ---------------------

const int WIDTH = 800;
const int HEIGHT = 600;

const int KEYS = GLFW_KEY_MENU + 1;

static bool pressed[KEYS]{ false };

class VEwindow {
protected:
	GLFWwindow* window;
public:
	void setUpWindow(const char* title, int = WIDTH, int = HEIGHT);
	void cleanUpWindow();

	void setWindowTitle(std::string);

	virtual void keyHandle();
	static void keyProcess(GLFWwindow*, int, int, int, int);
};

void key_callback(GLFWwindow*, int, int, int, int);

// ------------- VK Components ----------------

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#define VK_CHECK_RESULT(f)																							\
{																													\
	VkResult res = (f);																								\
	if (res != VK_SUCCESS)																							\
	{																												\
		std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << "\n";	\
		assert(res == VK_SUCCESS);																					\
	}																												\
}

const int MAX_FRAMES_IN_FLIGHT = 2;

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

class VEbase : public VEwindow {
private:
	const char* title;
protected:
	VkDebugUtilsMessengerEXT debugMessenger;
	
	vk::Instance instance;
	vk::PhysicalDevice physicalDevice;
	VkSurfaceKHR surface;

	vk::Device device;
	
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;

	vk::SwapchainKHR swapChain;
	vk::Format swapChainFormat;
	vk::Extent2D swapChainExtent;
	std::vector<vk::Image> swapChainImages;
	std::vector<vk::ImageView> swapChainImageViews;

	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;

	std::vector<vk::Fence> inflightFences;
	std::vector<vk::Semaphore> renderSemaphores;
	std::vector<vk::Semaphore> presentReadySemaphores;
public:
	VEbase(const char* title) {
		this->title = title;
	}

	bool framebufferResized = false;

	void init();
	void mainLoop();
	void cleanUpBase();

	void createInstance();
	void setUpDebugMessenger();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSurface();
	void createSwapChain();
	void createSwapChainImageViews();
	void recreateSwapChain();

	vk::SurfaceFormatKHR chooseSwapChainSurfaceFormat(std::vector<vk::SurfaceFormatKHR>);
	vk::PresentModeKHR chooseSwapChainPresentMode(std::vector<vk::PresentModeKHR>);
	vk::Extent2D chooseSwapChainExtent(const vk::SurfaceCapabilitiesKHR&);
	
	vk::ShaderModule createShaderModule(std::vector<char>&);
	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

	virtual void drawFrame();
	virtual void createFrameBuffers();
	virtual void destroyFrameBuffers();

	virtual void createFences();
	virtual void destroyFences();

	void createBuffer(vk::DeviceSize, vk::BufferUsageFlags, vk::MemoryPropertyFlags, vk::Buffer&, vk::DeviceMemory&);
	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
};

const std::string getShadersPath();

void updateInstanceExtensions();
bool checkValidationLayerSupport();
bool checkDeviceExtensionSupport(vk::PhysicalDevice);

bool isDeviceSuitable(vk::PhysicalDevice, vk::SurfaceKHR);
QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice, vk::SurfaceKHR);
SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice, vk::SurfaceKHR);

std::vector<char> readFileAsBinary(const std::string&);

// -------- Debug Messenger (Validation Layer) ---------

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT&);
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT,
	VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT*,
	void*);