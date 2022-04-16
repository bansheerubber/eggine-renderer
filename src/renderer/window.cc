#ifndef __switch__
#include <glad/gl.h>
#endif

#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../util/align.h"
#include "../engine/console.h"
// TODO re-enable in eggine repo
// #include "../engine/debug.h"
#include "debug.h"
// TODO re-enable in eggine repo
// #include "../engine/engine.h"
// #include "../resources/html.h"
#include "texture.h"
#include "window.h"

#include "shader.h"
#include "program.h"

#include "../resources/getShaderSource.h"

#ifndef __switch__
// TODO re-enable in eggine repo
// void onWindowResize(GLFWwindow* window, int width, int height) {
// 	engine->renderWindow.resize(width, height);
// }
#endif

// opengl: initialize GLFW, glEnables, etc
// deko3d: create framebuffers/swapchains, command buffers
void render::Window::initialize() {
	#ifdef __switch__ // start of switch code (based on switch-examples/graphics/deko3d/deko_basic)
	this->initializeDeko3d();
	#else // else for ifdef __switch__
	if(!glfwInit()) {
		console::error("failed to initialize glfw\n");
		exit(1);
	}

	// support 4.3
	if(this->backend == OPENGL_BACKEND) {
		this->initializeOpenGL();
	}
	else {
		this->initializeVulkan();
	}
	#endif // end for ifdef __switch__
}

// TODO re-enable in eggine repo
// void render::Window::initializeHTML() {
// 	this->htmlContainer = new LiteHTMLContainer();

// 	resources::HTML* html = (resources::HTML*)engine->manager->loadResources(engine->manager->carton->database.get()->equals("fileName", "html/index.html")->exec())[0];
// 	engine->manager->loadResources(engine->manager->carton->database.get()->equals("extension", ".css")->exec());
// 	this->htmlDocument = litehtml::document::createFromString(html->document.c_str(), this->htmlContainer, &this->htmlContext);
// }

void render::Window::addError() {
	this->errorCount++;
}

unsigned int render::Window::getErrorCount() {
	return this->errorCount;
}

void render::Window::clearErrors() {
	this->errorCount = 0;
}

void render::Window::registerHTMLUpdate() {
	this->htmlChecksum++;
}

void render::Window::setStencilFunction(render::StencilFunction func, unsigned int reference, unsigned int mask) {
	#ifdef __switch__
	#else
	if(this->backend == OPENGL_BACKEND) {
		glStencilFunc(stencilToGLStencil(func), reference, mask);
	}
	#endif
}

void render::Window::setStencilMask(unsigned int mask) {
	#ifdef __switch__
	#else
	if(this->backend == OPENGL_BACKEND) {
		glStencilMask(mask);
	}
	#endif
}

void render::Window::setStencilOperation(StencilOperation stencilFail, StencilOperation depthFail, StencilOperation pass) {
	#ifdef __switch__
	#else
	if(this->backend == OPENGL_BACKEND) {
		glStencilOp(stencilOPToGLStencilOP(stencilFail), stencilOPToGLStencilOP(depthFail), stencilOPToGLStencilOP(pass));
	}
	#endif
}

void render::Window::enableStencilTest(bool enable) {
	#ifdef __switch__
	#else
	if(this->backend == OPENGL_BACKEND) {
		if(enable) {
			glEnable(GL_STENCIL_TEST);
		}
		else {
			glDisable(GL_STENCIL_TEST);
		}
	}
	#endif
}

void render::Window::enableDepthTest(bool enable) {
	#ifdef __switch__
	#else
	if(this->backend == OPENGL_BACKEND) {
		if(enable) {
			glEnable(GL_DEPTH_TEST);
		}
		else {
			glDisable(GL_DEPTH_TEST);
		}
	}
	#endif
}

#ifdef __switch__
void render::Window::initializeDeko3d() {
	this->device = dk::DeviceMaker{}.create();
	this->queue = dk::QueueMaker{this->device}.setFlags(DkQueueFlags_Graphics).create();

	// start the construction of our framebuffers
	dk::ImageLayout framebufferLayout;
	dk::ImageLayoutMaker{this->device}
		.setFlags(DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression)
		.setFormat(DkImageFormat_RGBA8_Unorm)
		.setDimensions(this->width, this->height)
		.initialize(framebufferLayout); // helper data structure for framebuffer creation

	uint32_t framebufferSize  = framebufferLayout.getSize();
	uint32_t framebufferAlign = framebufferLayout.getAlignment();
	framebufferSize = alignTo(framebufferSize, framebufferAlign);

	// create the framebuffer's memory blocks from calculated size
	this->framebufferMemory = dk::MemBlockMaker{this->device, 2 * framebufferSize}.setFlags(DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image).create();

	// create images for framebuffers
	for(unsigned int i = 0; i < 2; i++) {
		this->framebuffers[i].initialize(framebufferLayout, this->framebufferMemory, i * framebufferSize);
	}

	// create the swapchain using the framebuffers
	std::array<DkImage const*, 2> framebufferArray = { &this->framebuffers[0], &this->framebuffers[1] };
	this->swapchain = dk::SwapchainMaker{this->device, nwindowGetDefault(), framebufferArray}.create();

	// create the memory that we'll use for the command buffer
	this->staticCommandBufferMemory = dk::MemBlockMaker{this->device, this->staticCommandBufferSize}.setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached).create();

	// create a buffer object for the command buffer
	this->staticCommandBuffer = dk::CmdBufMaker{this->device}.create();
	this->staticCommandBuffer.addMemory(this->staticCommandBufferMemory, 0, this->staticCommandBufferSize);

	// create command lists for our framebuffers, and also clarify them as render targets
	for(unsigned int i = 0; i < 2; i++) {
		dk::ImageView renderTarget { this->framebuffers[i] };
		this->staticCommandBuffer.bindRenderTargets(&renderTarget);
		this->framebufferCommandLists[i] = this->staticCommandBuffer.finishList();
	}

	this->blendState.setSrcColorBlendFactor(DkBlendFactor_SrcAlpha);
	this->blendState.setDstColorBlendFactor(DkBlendFactor_InvSrcAlpha);
	this->blendState.setSrcAlphaBlendFactor(DkBlendFactor_SrcAlpha);
	this->blendState.setDstAlphaBlendFactor(DkBlendFactor_InvSrcAlpha);

	this->colorState.setBlendEnable(0, true);

	this->rasterizerState.setCullMode(DkFace_None);

	// tell the switch that its time to disco
	this->staticCommandBuffer.setViewports(0, { this->viewport });
	this->staticCommandBuffer.setScissors(0, { this->scissor });
	this->staticCommandBuffer.clearColor(0, DkColorMask_RGBA, this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.a);
	this->staticCommandBuffer.bindRasterizerState(this->rasterizerState);
	this->staticCommandBuffer.bindColorState(this->colorState);
	this->staticCommandBuffer.bindColorWriteState(this->colorWriteState);
	this->staticCommandBuffer.bindBlendStates(0, this->blendState);
	this->staticCommandList = this->staticCommandBuffer.finishList();

	// create the dynamic command buffer
	// create the memory that we'll use for the command buffer
	this->commandBufferMemory = dk::MemBlockMaker{this->device, this->commandBufferSize}.setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached).create();

	// create a buffer object for the command buffer
	this->commandBuffer = dk::CmdBufMaker{this->device}.create();
	this->commandBuffer.addMemory(this->commandBufferMemory, this->commandBufferSliceSize * this->currentCommandBuffer, this->commandBufferSliceSize);

	// create a buffer object for the texture command buffer
	this->textureCommandBufferMemory = dk::MemBlockMaker{this->device, 4 * 1024}.setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached).create();
	this->textureCommandBuffer = dk::CmdBufMaker{this->device}.create();
	this->textureCommandBuffer.addMemory(this->textureCommandBufferMemory, 0, 4 * 1024);

	// create image/sampler descriptor memory
	this->imageDescriptorMemory = this->memory.allocate(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, sizeof(DkImageDescriptor) * IMAGE_SAMPLER_DESCRIPTOR_COUNT, DK_IMAGE_DESCRIPTOR_ALIGNMENT);

	this->samplerDescriptorMemory = this->memory.allocate(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, sizeof(DkSamplerDescriptor) * IMAGE_SAMPLER_DESCRIPTOR_COUNT, DK_SAMPLER_DESCRIPTOR_ALIGNMENT);

	// initialize gamepad
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);
	padInitializeDefault(&this->pad);
}
#else
void render::Window::initializeOpenGL() {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	this->window = glfwCreateWindow(this->width, this->height, "eggine", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSetErrorCallback(glfwErrorCallback);
	gladLoadGL(glfwGetProcAddress);

	// TODO re-enable in eggine repo
	// glfwSetWindowSizeCallback(this->window, onWindowResize);

	glEnable(GL_BLEND);
	this->enableDepthTest(true);
	this->enableStencilTest(true);
	// glEnable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glfwSwapInterval(1);

	#ifdef EGGINE_DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
	glDebugMessageCallback(glDebugOutput, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	#endif

	#ifdef EGGINE_DEVELOPER_MODE
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(this->window, false);
	ImGui_ImplOpenGL3_Init("#version 150");
	#endif
}

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

	// TODO re-enable in eggine repo
	// engine->manager->loadResources(engine->manager->carton->database.get()->equals("extension", ".spv")->exec());

	vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, this->device.graphicsQueueIndex);
	this->commandPool = this->device.device.createCommandPool(commandPoolInfo); 

	vk::CommandBufferAllocateInfo commandBufferInfo(this->commandPool, vk::CommandBufferLevel::ePrimary, 1);
	this->mainBuffer = this->device.device.allocateCommandBuffers(commandBufferInfo)[0];

	// create fences/semaphores
	vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);
	this->frameFence = this->device.device.createFence(fenceInfo);

	vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();
	this->isImageAvailable = this->device.device.createSemaphore(semaphoreInfo);
	this->isRenderFinished = this->device.device.createSemaphore(semaphoreInfo);

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
	result = this->device.device.createRenderPass(&renderPassInfo, nullptr, &this->renderPass); // TODO remember to clean up
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
#endif

void render::Window::deinitialize() {
	#ifdef __switch__
	this->queue.waitIdle();

	this->queue.destroy();
	this->staticCommandBuffer.destroy();
	this->staticCommandBufferMemory.destroy();
	this->commandBuffer.destroy();
	this->commandBufferMemory.destroy();
	this->swapchain.destroy();
	this->framebufferMemory.destroy();
	this->device.destroy();
	#else
	if(this->backend == VULKAN_BACKEND) {
		this->instance.destroySurfaceKHR(this->surface, nullptr);
		this->instance.destroy(nullptr);

		for(auto renderImageView: this->renderImageViews) {
			this->device.device.destroyImageView(renderImageView, nullptr);
		}

		this->device.device.destroySwapchainKHR(this->swapchain, nullptr);
		this->device.device.destroy(nullptr);
	}
	glfwTerminate();
	#endif
}

void render::Window::prerender() {
	int64_t startTime = getMicrosecondsNow();
	this->deltaTime = (startTime - this->lastRenderTime) / 1000000.0;
	this->lastRenderTime = getMicrosecondsNow();
	
	#ifdef __switch__
	// do dynamic command buffer magic
	this->commandBuffer.clear();
	this->commandBufferFences[this->signaledFence].wait();
	this->commandBuffer.addMemory(this->commandBufferMemory, this->commandBufferSliceSize * this->currentCommandBuffer, this->commandBufferSliceSize);

	// handle deallocated memory at the beginning of each frame
	this->memory.processDeallocationLists();

	// handle gamepad
	padUpdate(&this->pad);

	this->leftStick = padGetStickPos(&this->pad, 0);
	this->rightStick = padGetStickPos(&this->pad, 1);
	this->buttons = padGetButtons(&this->pad);
	#else
	if(this->backend == OPENGL_BACKEND) {
		glClearColor(this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.a);
		glStencilMask(0xFF);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
	else {
		// wait for the last fence to finish
		vk::Result result = this->device.device.waitForFences(1, &this->frameFence, true, UINT64_MAX);
		result = this->device.device.resetFences(1, &this->frameFence);
		if(result != vk::Result::eSuccess) {
			console::print("vulkan: failed to reset frame fence %s\n", vkResultToString((VkResult)result).c_str());
			exit(1);
		}

		// acquire the next image, signalling using the isImageAvailable semaphore
		this->currentFramebuffer = this->device.device.acquireNextImageKHR(this->swapchain, UINT64_MAX, this->isImageAvailable).value;
		
		// prepare the primary command buffer
		vk::CommandBufferBeginInfo bufferBeginInfo({}, nullptr);
		this->mainBuffer.reset();
		this->mainBuffer.begin(bufferBeginInfo);

		// only do one render pass for now
		vk::ClearValue clearColor(std::array<float, 4>({{ 0.0f, 0.0f, 0.0f, 1.0f }})); // ????
		vk::RenderPassBeginInfo renderPassInfo(this->renderPass, this->framebuffers[this->currentFramebuffer], { { 0, 0 }, this->swapchainExtent }, 1, &clearColor);

		this->mainBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
	}
	glfwPollEvents();
	this->hasGamepad = glfwGetGamepadState(GLFW_JOYSTICK_1, &this->gamepad);
	#endif
}

void render::Window::render() {
	// TODO re-enable in eggine repo
	// litehtml::position clip;
	// this->htmlContainer->get_client_rect(clip);
	
	// if(this->htmlChecksum != this->lastHTMLChecksum) {
	// 	this->htmlDocument->render(clip.width);
	// 	this->lastHTMLChecksum = this->htmlChecksum;
	// }
	
	// if(this->backend == OPENGL_BACKEND) {
	// 	this->htmlDocument->draw(0, 0, 0, nullptr);
	// }
	
	#ifdef __switch__
	int index = this->queue.acquireImage(this->swapchain);
	this->queue.submitCommands(this->framebufferCommandLists[index]);
	this->queue.submitCommands(this->staticCommandList);

	// do dynamic command buffer magic
	this->commandBuffer.signalFence(this->commandBufferFences[this->currentCommandBuffer]);
	this->signaledFence = this->currentCommandBuffer;
	this->queue.submitCommands(this->commandBuffer.finishList());

	this->currentCommandBuffer = (this->currentCommandBuffer + 1) % this->commandBufferCount;

	this->queue.presentImage(this->swapchain, index);
	#else
	// TODO re-enable in eggine repo
	// if(glfwWindowShouldClose(this->window)) {
	// 	engine->exit();
	// }
	
	if(this->backend == OPENGL_BACKEND) {
		glfwSwapBuffers(this->window);
	}
	else {
		// finalize render pass
		this->mainBuffer.endRenderPass();
		this->mainBuffer.end();

		// submit the image for presentation
		vk::Semaphore waitSemaphores[] = { this->isImageAvailable };
		vk::Semaphore signalSemaphores[] = { this->isRenderFinished };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::SubmitInfo submitInfo(1, waitSemaphores, waitStages, 1, &this->mainBuffer, 1, signalSemaphores);

		vk::Result result = this->graphicsQueue.submit(1, &submitInfo, this->frameFence);
		if(result != vk::Result::eSuccess) {
			console::print("vulkan: failed to submit graphics queue %s\n", vkResultToString((VkResult)result).c_str());
			exit(1);
		}

		// present the image. wait for the queue's submit signal
		vk::PresentInfoKHR presentInfo(1, signalSemaphores, 1, &this->swapchain, &this->currentFramebuffer, nullptr);
		result = this->presentationQueue.presentKHR(presentInfo);
		if(result != vk::Result::eSuccess) {
			console::print("vulkan: failed to present via presentation queue %s\n", vkResultToString((VkResult)result).c_str());
			exit(1);
		}
	}
	#endif
}

void render::Window::draw(PrimitiveType type, unsigned int firstVertex, unsigned int vertexCount, unsigned int firstInstance, unsigned int instanceCount) {
	#ifdef __switch__
	this->commandBuffer.draw(primitiveToDkPrimitive(type), vertexCount, instanceCount, firstVertex, firstInstance);
	#else
	if(this->backend == OPENGL_BACKEND) {
		if(firstInstance == 0 && instanceCount == 1) {
			glDrawArrays(primitiveToGLPrimitive(type), firstVertex, vertexCount);
		}
		else {
			glDrawArraysInstancedBaseInstance(primitiveToGLPrimitive(type), firstVertex, vertexCount, instanceCount, firstInstance);
		}
	}
	#endif
}

void render::Window::resize(unsigned int width, unsigned int height) {
	this->registerHTMLUpdate();
	this->width = width;
	this->height = height;
	
	#ifndef __switch__
	if(this->backend == OPENGL_BACKEND) {
		glViewport(0, 0, width, height);
	}
	#endif
}

#ifdef __switch__
void render::Window::addTexture(switch_memory::Piece* tempMemory, dk::ImageView& view, unsigned int width, unsigned int height) {
	this->queue.waitIdle();
	this->textureCommandBuffer.clear();
	this->textureCommandBuffer.addMemory(this->textureCommandBufferMemory, 0, 4 * 1024);

	this->textureCommandBuffer.copyBufferToImage({ tempMemory->gpuAddr() }, view, { 0, 0, 0, width, height, 1 });
	this->queue.submitCommands(this->textureCommandBuffer.finishList());
	this->queue.waitIdle(); // wait to add the texture
	this->textureCommandBuffer.clear();
}

void render::Window::bindTexture(unsigned int location, Texture* texture) {
	this->commandBuffer.pushData(this->imageDescriptorMemory->gpuAddr() + location * sizeof(DkImageDescriptor), &texture->imageDescriptor, sizeof(DkImageDescriptor));
	this->commandBuffer.pushData(this->samplerDescriptorMemory->gpuAddr() + location * sizeof(DkSamplerDescriptor), &texture->samplerDescriptor, sizeof(DkSamplerDescriptor));

	this->commandBuffer.bindImageDescriptorSet(this->imageDescriptorMemory->gpuAddr(), IMAGE_SAMPLER_DESCRIPTOR_COUNT);
	this->commandBuffer.bindSamplerDescriptorSet(this->samplerDescriptorMemory->gpuAddr(), IMAGE_SAMPLER_DESCRIPTOR_COUNT);
}
#else
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
}
#endif
