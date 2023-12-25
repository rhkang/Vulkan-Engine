cmake_minimum_required (VERSION 3.8)

set(NAME "VulkanEngine")
project (${NAME})

include_directories(include)
include_directories(include/glm)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/base)

add_definitions(-DSHADERS_DIR=\"${CMAKE_SOURCE_DIR}/shaders/\")

find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs)
find_library(glfw_LIBRARY NAMES glfw3 PATHS ${CMAKE_SOURCE_DIR}/libs)

include_directories(base)
include_directories(temp)
include_directories(shaders)

add_subdirectory (example)
