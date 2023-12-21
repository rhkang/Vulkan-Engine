#pragma once

#include "VEwindow.h"

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include "GLFW/glfw3.h"

#include <iostream>
#include <optional>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;

	bool isComplete() {
		return graphicsFamily.has_value();
	}
};

class VEbase : public VEwindow {
private:
	const char* title;
protected:
	VkDebugUtilsMessengerEXT debugMessenger;
	
	vk::Instance instance;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	vk::Queue graphicsQueue;
public:
	VEbase(const char* title) {
		this->title = title;
	}

	void init();
	void mainLoop();
	void cleanUp();

	void createInstance();
	void setUpDebugMessenger();
	void pickPhysicalDevice();
	void createLogicalDevice();
};

void updateInstanceExtensions();
bool checkValidationLayerSupport();

bool isDeviceSuitable(vk::PhysicalDevice);
QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice);

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT&);
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT,
	VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT*,
	void*);