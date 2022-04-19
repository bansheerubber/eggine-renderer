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
#include "program.h"
#include "shader.h"
#include "texture.h"
#include "window.h"

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

render::State &render::Window::getState(uint32_t id) {
	// id 0 is special, this represents the main command buffer
	if(this->renderStates.find(id) == this->renderStates.end()) {
		this->renderStates[id] = State(this); // TODO create command buffer for vulkan
	}
	return this->renderStates[id];
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
	this->commandBuffer.addMemory(this->commandBufferMemory, this->commandBufferSliceSize * this->framePingPong, this->commandBufferSliceSize);

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
	// this->enableDepthTest(true);
	// this->enableStencilTest(true);
	glEnable(GL_CULL_FACE);
	glEnable(GL_FRAMEBUFFER_SRGB);
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
		this->device.device.waitIdle();

		for(Program* program: this->programs) {
			for(Shader* shader: program->shaders) {
				this->device.device.destroyShaderModule(shader->module);
			}
		}

		for(auto &[_, pipeline]: this->pipelineCache) {
			this->device.device.destroyPipelineLayout(*pipeline.layout);
			this->device.device.destroyPipeline(*pipeline.pipeline);
		}

		this->device.device.destroyRenderPass(this->renderPass);

		for(auto framebuffer: this->framebuffers) {
			this->device.device.destroyFramebuffer(framebuffer);
		}

		this->device.device.destroySwapchainKHR(this->swapchain);
		this->instance.destroySurfaceKHR(this->surface);

		for(auto renderImageView: this->renderImageViews) {
			this->device.device.destroyImageView(renderImageView);
		}

		for(uint8_t i = 0; i < 2; i++) {
			this->device.device.destroySemaphore(this->isRenderFinished[i]);
			this->device.device.destroySemaphore(this->isImageAvailable[i]);
			this->device.device.destroyFence(this->frameFence[i]);
		}

		this->device.device.destroyCommandPool(this->commandPool);
		this->device.device.destroyDescriptorPool(this->descriptorPool);

		this->device.device.destroy();

		this->instance.destroyDebugUtilsMessengerEXT(this->debugCallback, nullptr, vk::DispatchLoaderDynamic{ this->instance, vkGetInstanceProcAddr });
		this->instance.destroy();
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
	this->commandBuffer.addMemory(this->commandBufferMemory, this->commandBufferSliceSize * this->framePingPong, this->commandBufferSliceSize);

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
		if(this->swapchainOutOfDate) {
			this->createSwapchain();
			this->swapchainOutOfDate = false;
		}
		
		// wait for the last fence to finish
		vk::Result result = this->device.device.waitForFences(1, &this->frameFence[this->framePingPong], true, UINT64_MAX);
		if(result != vk::Result::eSuccess) {
			console::print("vulkan: failed to wait on frame fence %s\n", vkResultToString((VkResult)result).c_str());
			exit(1);
		}

		// acquire the next image, signalling using the isImageAvailable semaphore
		result = this->device.device.acquireNextImageKHR(this->swapchain, UINT64_MAX, this->isImageAvailable[this->framePingPong], {}, &this->currentFramebuffer);
		if(result == vk::Result::eErrorOutOfDateKHR) {
			this->swapchainOutOfDate = true;
			return;
		}

		result = this->device.device.resetFences(1, &this->frameFence[this->framePingPong]);
		if(result != vk::Result::eSuccess) {
			console::print("vulkan: failed to reset frame fence %s\n", vkResultToString((VkResult)result).c_str());
			exit(1);
		}
		
		// prepare the primary command buffer
		vk::CommandBufferBeginInfo bufferBeginInfo({}, nullptr);
		this->renderStates[0].reset();
		this->renderStates[0].buffer[this->framePingPong].begin(bufferBeginInfo);

		// only do one render pass for now
		vk::ClearValue clearColor(
			std::array<float, 4>({{ this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.a }})
		); // ????
		vk::RenderPassBeginInfo renderPassInfo(this->renderPass, this->framebuffers[this->currentFramebuffer], { { 0, 0 }, this->swapchainExtent }, 1, &clearColor);

		this->renderStates[0].buffer[this->framePingPong].beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
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
	this->commandBuffer.signalFence(this->commandBufferFences[this->framePingPong]);
	this->signaledFence = this->framePingPong;
	this->queue.submitCommands(this->commandBuffer.finishList());

	this->framePingPong = (this->framePingPong + 1) % this->commandBufferCount;

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
		if(this->swapchainOutOfDate) {
			return;
		}
		
		// finalize render pass
		this->renderStates[0].buffer[this->framePingPong].endRenderPass();
		this->renderStates[0].buffer[this->framePingPong].end();

		// we need to wait for any copy operations to finish before we process main command buffer
		if(this->memoryCopyFences.size() > 0) {
			console::print("begin the wait\n");
			vk::Result result = this->device.device.waitForFences(this->memoryCopyFences.size(), this->memoryCopyFences.data(), true, UINT64_MAX);
			if(result != vk::Result::eSuccess) {
				console::print("vulkan: failed to wait for memory transfer fences %s\n", vkResultToString((VkResult)result).c_str());
				exit(1);
			}
			console::print("wait finished\n");
		}

		// submit the image for presentation
		vk::Semaphore waitSemaphores[] = { this->isImageAvailable[this->framePingPong] };
		vk::Semaphore signalSemaphores[] = { this->isRenderFinished[this->framePingPong] };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::SubmitInfo submitInfo(1, waitSemaphores, waitStages, 1, &this->renderStates[0].buffer[this->framePingPong], 1, signalSemaphores);

		vk::Result result = this->graphicsQueue.submit(1, &submitInfo, this->frameFence[this->framePingPong]);
		if(result != vk::Result::eSuccess) {
			console::print("vulkan: failed to submit graphics queue %s\n", vkResultToString((VkResult)result).c_str());
			exit(1);
		}

		// present the image. wait for the queue's submit signal
		vk::PresentInfoKHR presentInfo(1, signalSemaphores, 1, &this->swapchain, &this->currentFramebuffer, nullptr);
		result = this->presentationQueue.presentKHR(&presentInfo);
		if(result == vk::Result::eErrorOutOfDateKHR) {
			this->swapchainOutOfDate = true;
			return;
		}
		else if(result != vk::Result::eSuccess) {
			console::print("vulkan: failed to present via presentation queue %s\n", vkResultToString((VkResult)result).c_str());
			exit(1);
		}

		// end copy operations
		for(vk::Fence fence: this->memoryCopyFences) {
			this->device.device.destroyFence(fence);
		}
		this->memoryCopyFences.clear();

		if(this->memoryCopyCommandBuffers.size() > 0) {
			this->device.device.freeCommandBuffers(
				this->commandPool, this->memoryCopyCommandBuffers.size(), this->memoryCopyCommandBuffers.data()
			);
			this->memoryCopyCommandBuffers.clear();
		}

		this->framePingPong = (this->framePingPong + 1) % 2;
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
void render::Window::addTexture(Piece* tempMemory, dk::ImageView& view, unsigned int width, unsigned int height) {
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
#endif
