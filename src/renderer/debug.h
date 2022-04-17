#pragma once

#ifndef __switch__
#include <GLFW/glfw3.h>
#endif

#include <tsl/robin_map.h>
#include <string>

#ifndef __switch__
inline std::string vkResultToString(VkResult result) {
	switch(result) {
		case VK_SUCCESS: return std::string("VK_SUCCESS");
    case VK_NOT_READY: return std::string("VK_NOT_READY");
    case VK_TIMEOUT: return std::string("VK_TIMEOUT");
    case VK_EVENT_SET: return std::string("VK_EVENT_SET");
    case VK_EVENT_RESET: return std::string("VK_EVENT_RESET");
    case VK_INCOMPLETE: return std::string("VK_INCOMPLETE");
    case VK_ERROR_OUT_OF_HOST_MEMORY: return std::string("VK_ERROR_OUT_OF_HOST_MEMORY");
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return std::string("VK_ERROR_OUT_OF_DEVICE_MEMORY");
    case VK_ERROR_INITIALIZATION_FAILED: return std::string("VK_ERROR_INITIALIZATION_FAILED");
    case VK_ERROR_DEVICE_LOST: return std::string("VK_ERROR_DEVICE_LOST");
    case VK_ERROR_MEMORY_MAP_FAILED: return std::string("VK_ERROR_MEMORY_MAP_FAILED");
    case VK_ERROR_LAYER_NOT_PRESENT: return std::string("VK_ERROR_LAYER_NOT_PRESENT");
    case VK_ERROR_EXTENSION_NOT_PRESENT: return std::string("VK_ERROR_LAYER_NOT_PRESENT");
    case VK_ERROR_FEATURE_NOT_PRESENT: return std::string("VK_ERROR_FEATURE_NOT_PRESENT");
    case VK_ERROR_INCOMPATIBLE_DRIVER: return std::string("VK_ERROR_INCOMPATIBLE_DRIVER");
    case VK_ERROR_TOO_MANY_OBJECTS: return std::string("VK_ERROR_TOO_MANY_OBJECTS");
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return std::string("VK_ERROR_FORMAT_NOT_SUPPORTED");
    case VK_ERROR_FRAGMENTED_POOL: return std::string("VK_ERROR_FRAGMENTED_POOL");
    case VK_ERROR_UNKNOWN: return std::string("VK_ERROR_UNKNOWN");
    case VK_ERROR_OUT_OF_POOL_MEMORY: return std::string("VK_ERROR_OUT_OF_POOL_MEMORY");
    case VK_ERROR_INVALID_EXTERNAL_HANDLE: return std::string("VK_ERROR_INVALID_EXTERNAL_HANDLE");
    case VK_ERROR_FRAGMENTATION: return std::string("VK_ERROR_FRAGMENTATION");
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return std::string("VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS");
    case VK_PIPELINE_COMPILE_REQUIRED: return std::string("VK_PIPELINE_COMPILE_REQUIRED");
    case VK_ERROR_SURFACE_LOST_KHR: return std::string("VK_ERROR_SURFACE_LOST_KHR");
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return std::string("VK_ERROR_NATIVE_WINDOW_IN_USE_KHR");
    case VK_SUBOPTIMAL_KHR: return std::string("VK_SUBOPTIMAL_KHR");
    case VK_ERROR_OUT_OF_DATE_KHR: return std::string("VK_ERROR_OUT_OF_DATE_KHR");
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return std::string("VK_ERROR_INCOMPATIBLE_DISPLAY_KHR");
    case VK_ERROR_VALIDATION_FAILED_EXT: return std::string("VK_ERROR_VALIDATION_FAILED_EXT");
    case VK_ERROR_INVALID_SHADER_NV: return std::string("VK_ERROR_INVALID_SHADER_NV");
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return std::string("VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT");
    case VK_ERROR_NOT_PERMITTED_KHR: return std::string("VK_ERROR_NOT_PERMITTED_KHR");
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return std::string("VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT");
    case VK_THREAD_IDLE_KHR: return std::string("VK_THREAD_IDLE_KHR");
    case VK_THREAD_DONE_KHR: return std::string("VK_THREAD_DONE_KHR");
    case VK_OPERATION_DEFERRED_KHR: return std::string("VK_OPERATION_DEFERRED_KHR");
    case VK_OPERATION_NOT_DEFERRED_KHR: return std::string("VK_OPERATION_NOT_DEFERRED_KHR");
		default: return std::string("VK_UNKNOWN_RESULT");
	}
}

inline void glfwErrorCallback(int errorCode, const char* message) {
  console::print("glfw error(%d): %s\n", errorCode, message);
}

inline VkBool32 vulkanDebugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageTypes,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData
) {
  if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
    console::print("vulkan verbose: %s\n", pCallbackData->pMessage);
  }
  else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    console::print("vulkan info: %s\n", pCallbackData->pMessage);
  }
  else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    console::warning("vulkan warning: %s\n", pCallbackData->pMessage);
  }
  else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    console::error("vulkan error: %s\n", pCallbackData->pMessage);
  }

  return VK_FALSE;
}
#endif
