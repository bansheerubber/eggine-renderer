#ifndef __switch__
#include <glad/gl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "window.h"

// opengl: initialize GLFW, glEnables, etc
// deko3d: create framebuffers/swapchains, command buffers
void render::Window::initialize() {
	#ifdef __switch__ // start of switch code (based on switch-examples/graphics/deko3d/deko_basic)
	socketInitializeDefault();
	this->nxlink = nxlinkStdio();

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
	framebufferSize = (framebufferSize + framebufferAlign - 1) & ~(framebufferAlign - 1);

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
	this->commandBufferMemory = dk::MemBlockMaker{this->device, this->commandBufferSize}.setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached).create();

	// create a buffer object for the command buffer
	this->staticCommandBuffer = dk::CmdBufMaker{this->device}.create();
	this->staticCommandBuffer.addMemory(this->commandBufferMemory, 0, this->commandBufferSize);

	// create command lists for our framebuffers, and also clarify them as render targets
	for(unsigned int i = 0; i < 2; i++) {
		dk::ImageView renderTarget { this->framebuffers[i] };
		this->staticCommandBuffer.bindRenderTargets(&renderTarget);
		this->framebufferCommandLists[i] = this->staticCommandBuffer.finishList();
	}

	// tell the switch that its time to disco
	this->staticCommandBuffer.setViewports(0, { this->viewport });
	this->staticCommandBuffer.setScissors(0, { this->scissor });
	this->staticCommandBuffer.clearColor(0, DkColorMask_RGBA, this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.a);
	this->staticCommandBuffer.bindRasterizerState(this->rasterizerState);
	this->staticCommandBuffer.bindColorState(this->colorState);
	this->staticCommandBuffer.bindColorWriteState(this->colorWriteState);
	this->staticCommandList = this->staticCommandBuffer.finishList();

	#else // else for ifdef __switch__
	if(!glfwInit()) {
		printf("failed to initialize glfw\n");
	}
	else {
		printf("initialized glfw\n");
	}

	// support 4.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	this->window = glfwCreateWindow(this->width, this->height, "eggine", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGL(glfwGetProcAddress);

	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glfwSwapInterval(1);

	/* TODO enable during copy/paste into eggine
	#ifdef EGGINE_DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
	glDebugMessageCallback(glDebugOutput, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	#endif
	*/

	#endif // end for ifdef __switch__
}

void render::Window::deinitialize() {
	#ifdef __switch__
	this->queue.waitIdle();

	this->queue.destroy();
	this->staticCommandBuffer.destroy();
	this->commandBufferMemory.destroy();
	this->swapchain.destroy();
	this->framebufferMemory.destroy();
	this->device.destroy();

	close(this->nxlink);
	socketExit();
	#else
	glfwTerminate();
	#endif
}

void render::Window::prerender() {
	long long startTime = getMicrosecondsNow();
	this->deltaTime = (startTime - this->lastRenderTime) / 1000000.0;
	this->lastRenderTime = getMicrosecondsNow();
	
	#ifdef __switch__
	#else
	glClearColor(this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glfwPollEvents();
	#endif
}

void render::Window::render() {
	#ifdef __switch__
	int index = this->queue.acquireImage(this->swapchain);
	this->queue.submitCommands(this->framebufferCommandLists[index]);
	this->queue.submitCommands(this->staticCommandList);
	this->queue.presentImage(this->swapchain, index);
	#else
	glfwSwapBuffers(this->window);
	#endif
}

void render::Window::resize(unsigned int width, unsigned int height) {
	#ifndef __switch__
	glViewport(0, 0, width, height);
	#endif
}
