#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../util/align.h"
#include "../engine/console.h"
#include "debug.h"
#include "program.h"
#include "shader.h"
#include "texture.h"
#include "window.h"

#ifndef __switch__
void render::Window::initializeVulkan() {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	this->window = glfwCreateWindow(this->width, this->height, "eggine", NULL, NULL);
	glfwSetErrorCallback(glfwErrorCallback);

	vk::ApplicationInfo app(
		"VulkanClear", VK_MAKE_VERSION(1, 0, 0), "ClearScreenEngine", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_3
	);

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // TODO turn this into std::string so we can add our own extensions?

	std::vector<const char*> extensions;
	for(uint32_t i = 0; i < glfwExtensionCount; i++) {
		extensions.push_back(glfwExtensions[i]);
	}

	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	extensions.push_back("VK_KHR_xlib_surface");
	extensions.push_back("VK_KHR_display");

	vk::InstanceCreateInfo createInfo(
		{}, &app, (uint32_t)RequiredValidationLayers.size(), RequiredValidationLayers.data(), (uint32_t)extensions.size(), extensions.data()
	);

	vk::Result result = vk::createInstance(&createInfo, nullptr, &this->instance);
	if(result != vk::Result::eSuccess) {
		console::error("vulkan: could not create instance: %s\n", vkResultToString((VkResult)result).c_str());
		exit(1);
	}

	// handle debug
	vk::DebugUtilsMessengerCreateInfoEXT debugInfo(
		{},
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
		vulkanDebugCallback
	);
	
	result = this->instance.createDebugUtilsMessengerEXT(&debugInfo, nullptr, &this->debugCallback, vk::DispatchLoaderDynamic{ this->instance, vkGetInstanceProcAddr });
	if(result != vk::Result::eSuccess) {
		console::error("vulkan: could not create debug callback: %s\n", vkResultToString((VkResult)result).c_str());
		exit(1);
	}

	// handle surface
	VkSurfaceKHR surface;
	VkResult surfaceResult = glfwCreateWindowSurface(this->instance, this->window, nullptr, &surface);
	if(surfaceResult != VK_SUCCESS || surface == VK_NULL_HANDLE) {
		console::error("vulkan: could not create window surface: %s\n", vkResultToString((VkResult)surfaceResult).c_str());
		exit(1);
	}

	this->surface = surface;

	// handle physical devices
	this->pickDevice();
	this->setupDevice();
	this->createSwapchain();

	// TODO re-enable in eggine repo
	// engine->manager->loadResources(engine->manager->carton->database.get()->equals("extension", ".spv")->exec());

	vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, this->device.graphicsQueueIndex);
	this->commandPool = this->device.device.createCommandPool(commandPoolInfo); 

	vk::CommandBufferAllocateInfo commandBufferInfo(this->commandPool, vk::CommandBufferLevel::ePrimary, 2);
	this->renderStates[0] = State(this);
	result = this->device.device.allocateCommandBuffers(&commandBufferInfo, this->renderStates[0].buffer);
	if(result != vk::Result::eSuccess) {
		console::error("vulkan: could not create command buffers: %s\n", vkResultToString((VkResult)result).c_str());
		exit(1);
	}

	// create fences/semaphores
	vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);
	vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();
	for(uint8_t i = 0; i < 2; i++) {
		this->frameFence[i] = this->device.device.createFence(fenceInfo);
		this->isImageAvailable[i] = this->device.device.createSemaphore(semaphoreInfo);
		this->isRenderFinished[i] = this->device.device.createSemaphore(semaphoreInfo);
	}

	// create descriptor pool
	std::vector<vk::DescriptorPoolSize> poolSizes;
	poolSizes.push_back(vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1)); // TODO change to 2, also maybe revisit for textures?
	vk::DescriptorPoolCreateInfo descriptorInfo({}, 1, (uint32_t)poolSizes.size(), poolSizes.data()); // also change 1 to 2
	this->descriptorPool = this->device.device.createDescriptorPool(descriptorInfo);
}

void render::Window::createSwapchain() {
	if(this->pipelineCache.size() > 0) {
		this->device.device.waitIdle();
		
		for(auto &[_, pipeline]: this->pipelineCache) { // have to destroy all pipelines
			this->device.device.destroyPipelineLayout(*pipeline.layout);
			this->device.device.destroyPipeline(*pipeline.pipeline);
		}

		this->pipelineCache.clear();

		this->device.device.destroyRenderPass(this->renderPass);

		for(auto framebuffer: this->framebuffers) {
			this->device.device.destroyFramebuffer(framebuffer);
		}

		this->device.device.destroySwapchainKHR(this->swapchain);

		for(auto renderImageView: this->renderImageViews) {
			this->device.device.destroyImageView(renderImageView);
		}
	}

	// update device
	this->device.surfaceFormats = this->device.physicalDevice.getSurfaceFormatsKHR(this->surface);
	this->device.capabilities = this->device.physicalDevice.getSurfaceCapabilitiesKHR(this->surface);

	// now create the swapchain
	vk::SurfaceFormatKHR format(vk::Format::eUndefined);
	for(vk::SurfaceFormatKHR f: this->device.surfaceFormats) {
		if(f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			format = f;
		}
	}

	if(format.format == vk::Format::eUndefined) {
		console::print("vulkan: could not find suitable surface format\n");
		exit(1);
	}

	uint32_t width, height;
	glfwGetFramebufferSize(this->window, (int*)&width, (int*)&height);

	width = std::clamp(width, this->device.capabilities.minImageExtent.width, this->device.capabilities.maxImageExtent.width);
	height = std::clamp(width, this->device.capabilities.minImageExtent.height, this->device.capabilities.maxImageExtent.height);

	this->width = width;
	this->height = height;

	this->swapchainExtent = vk::Extent2D(width, height);
	this->swapchainFormat = format;

	uint32_t imageCount = 2; // TODO this needs to be double checked against the drivers
	vk::SwapchainCreateInfoKHR swapChainInfo(
		{},
		this->surface,
		imageCount,
		format.format,
		format.colorSpace,
		this->swapchainExtent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive,
		{},
		nullptr,
		this->device.capabilities.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		vk::PresentModeKHR::eFifo,
		true
	);

	// handle multiple queues
	if(this->device.graphicsQueueIndex != this->device.presentationQueueIndex) {
		swapChainInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapChainInfo.queueFamilyIndexCount = 2;
		swapChainInfo.pQueueFamilyIndices = &this->device.graphicsQueueIndex; // i dare you to crash vulkan
	}

	this->swapchain = this->device.device.createSwapchainKHR(swapChainInfo);
	this->renderImages = this->device.device.getSwapchainImagesKHR(this->swapchain);

	// create image views
	this->renderImageViews.resize(imageCount);
	for(uint32_t i = 0; i < imageCount; i++) {
		vk::ImageViewCreateInfo imageViewInfo(
			{},
			this->renderImages[i],
			vk::ImageViewType::e2D,
			format.format,
			{ vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, },
			{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
		);

		this->renderImageViews[i] = this->device.device.createImageView(imageViewInfo);
	}

	// handle renderpass creation
	vk::AttachmentDescription colorAttachment(
		{},
		this->swapchainFormat.format,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear, // color
		vk::AttachmentStoreOp::eStore, // color
		vk::AttachmentLoadOp::eDontCare, // stencil
		vk::AttachmentStoreOp::eDontCare, // stencil
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::ePresentSrcKHR
	);

	vk::AttachmentReference attachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal); // attach to the output color location in the shader
	vk::SubpassDescription subpass(
		{},
		vk::PipelineBindPoint::eGraphics,
		0,
		nullptr,
		1,
		&attachmentReference
	);

	// create a subpass dependency that waits for color
	vk::SubpassDependency dependency(
		VK_SUBPASS_EXTERNAL, // src
		0, // dest
		vk::PipelineStageFlagBits::eColorAttachmentOutput, // src stage
		vk::PipelineStageFlagBits::eColorAttachmentOutput, // dest stage
		{}, // src access
		vk::AccessFlagBits::eColorAttachmentWrite // dest access
	);

	vk::RenderPassCreateInfo renderPassInfo({}, 1, &colorAttachment, 1, &subpass, 1, &dependency);
	vk::Result result = this->device.device.createRenderPass(&renderPassInfo, nullptr, &this->renderPass); // TODO remember to clean up
	if(result != vk::Result::eSuccess) {
		console::error("vulkan: could not create render pass: %s\n", vkResultToString((VkResult)result).c_str());
		exit(1);
	}

	this->framebuffers.resize(this->renderImageViews.size());
	for(size_t i = 0; i < this->renderImageViews.size(); i++) {
		vk::ImageView attachments[] = { this->renderImageViews[i] };
		vk::FramebufferCreateInfo framebufferInfo(
			{},
			this->renderPass,
			1,
			attachments,
			this->swapchainExtent.width,
			this->swapchainExtent.height,
			1
		);

		vk::Result result = this->device.device.createFramebuffer(&framebufferInfo, nullptr, &this->framebuffers[i]);
		if(result != vk::Result::eSuccess) {
			console::error("vulkan: could not create framebuffers: %s\n", vkResultToString((VkResult)result).c_str());
			exit(1);
		}
	}
}

void render::Window::pickDevice() {
	std::vector<vk::PhysicalDevice> devices = this->instance.enumeratePhysicalDevices();

	Device integratedGPU = { VK_NULL_HANDLE, VK_NULL_HANDLE };
	Device discreteGPU = { VK_NULL_HANDLE, VK_NULL_HANDLE };
	for(vk::PhysicalDevice device: devices) {
		Device potentialDevice = {
			VK_NULL_HANDLE, device, {}, {}, (uint32_t)-1, (uint32_t)-1,
		};

		potentialDevice.properties = device.getProperties(); // devices properties (name, etc)
		potentialDevice.features = device.getFeatures(); // device features (texture sizes, supported shaders, etc)

		// handle searching for queues
		{
			std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
			for(size_t i = 0; i < queueFamilies.size(); i++) {
				if((queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) && potentialDevice.graphicsQueueIndex == (uint32_t)-1) {
					potentialDevice.graphicsQueueIndex = i;
				}

				bool presentSupport = device.getSurfaceSupportKHR(i, this->surface);
				if(presentSupport && potentialDevice.presentationQueueIndex == (uint32_t)-1) {
					potentialDevice.presentationQueueIndex = i;
				}
			}

			if(potentialDevice.graphicsQueueIndex == (uint32_t)-1 || potentialDevice.presentationQueueIndex == (uint32_t)-1) {
				console::print("skipped because no queues\n");
				continue;
			}
		}

		// handle required extensions
		{
			std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();
			uint32_t foundExtensions = 0;
			for(auto &extension: availableExtensions) {
				if(std::find_if(
					RequiredDeviceExtensions.begin(),
					RequiredDeviceExtensions.end(),
					[extension] (const char* s) { return std::string(s) == std::string(extension.extensionName); }
				) != RequiredDeviceExtensions.end()) {
					foundExtensions++;
				}
			}

			if(foundExtensions != RequiredDeviceExtensions.size()) {
				console::print("skipped because no extensions\n");
				continue;
			}
		}

		// handle swapchain support
		{
			potentialDevice.surfaceFormats = device.getSurfaceFormatsKHR(this->surface);
			if(potentialDevice.surfaceFormats.size() == 0) {
				console::print("skipped because no surface formats\n");
				continue;
			}

			potentialDevice.presentModes = device.getSurfacePresentModesKHR(this->surface);
			if(potentialDevice.presentModes.size() == 0) {
				console::print("skipped because no present modes\n");
				continue;
			}

			potentialDevice.capabilities = device.getSurfaceCapabilitiesKHR(this->surface);
		}

		// finally, we are done
		if(
			potentialDevice.properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu
			|| potentialDevice.properties.deviceType == vk::PhysicalDeviceType::eCpu
		) {
			integratedGPU = potentialDevice;
		}
		else if(potentialDevice.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu
			|| potentialDevice.properties.deviceType == vk::PhysicalDeviceType::eVirtualGpu
		) {
			discreteGPU = potentialDevice;
		}
	}

	Device selectedDevice = !discreteGPU.physicalDevice ? integratedGPU : discreteGPU;
	if(!selectedDevice.physicalDevice) {
		console::print("vulkan: could not find suitable display device\n");
		exit(1);
	}

	console::print("vulkan: selected device '%s'\n", selectedDevice.properties.deviceName.data());
	this->device = selectedDevice;
}

void render::Window::setupDevice() {
	float queuePriority = 1.0f;
	std::set<uint32_t> queues = { this->device.graphicsQueueIndex, this->device.presentationQueueIndex };
	std::vector<vk::DeviceQueueCreateInfo> creationInfos;
	for(auto queue: queues) {
		creationInfos.push_back(vk::DeviceQueueCreateInfo(
			{},
			queue,
			1,
			&queuePriority
		));
	}

	vk::DeviceCreateInfo deviceCreateInfo(
		{},
		(uint32_t)creationInfos.size(),
		creationInfos.data(),
		0,
		nullptr,
		(uint32_t)RequiredDeviceExtensions.size(),
		RequiredDeviceExtensions.data()
	);
	
	this->device.device = this->device.physicalDevice.createDevice(deviceCreateInfo);

	this->graphicsQueue = this->device.device.getQueue(this->device.graphicsQueueIndex, 0);
	this->presentationQueue = this->device.device.getQueue(this->device.presentationQueueIndex, 0);
}

uint32_t render::Window::findVulkanMemoryType(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags propertyFlags) {
	vk::PhysicalDeviceMemoryProperties properties = this->device.physicalDevice.getMemoryProperties();
	for(uint32_t i = 0; i < properties.memoryTypeCount; i++) {
		if((requirements.memoryTypeBits & (1 << i)) && (properties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
			return i;
		}
	}

	return -1;
}
#endif
