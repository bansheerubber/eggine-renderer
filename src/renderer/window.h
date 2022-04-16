#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#include <switch.h>
#else
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#endif

// TODO re-enable in eggine repo
// #include "../engine/developer.h"

// TODO re-enable in eggine repo
// #include <litehtml.h>
#include <tsl/robin_map.h>
#include <glm/vec4.hpp>
#include <vector>

#include "../util/crop.h"
// TODO re-enable in eggine repo
// #include "../renderer/litehtmlContainer.h"
#include "memory.h"
#include "pipeline.h"
#include "../util/png.h"
#include "primitive.h"
#include "state.h"
#include "texture.h"
#include "../util/time.h"

#include "stencil.h"

namespace render {
	#ifndef __switch__
	struct Device {
		vk::Device device;
		vk::PhysicalDevice physicalDevice;
		vk::PhysicalDeviceProperties properties;
		vk::PhysicalDeviceFeatures features;
		uint32_t graphicsQueueIndex;
		uint32_t presentationQueueIndex;
		std::vector<vk::SurfaceFormatKHR> surfaceFormats;
		std::vector<vk::PresentModeKHR> presentModes;
		vk::SurfaceCapabilitiesKHR capabilities;
	};

	const std::vector<const char*> RequiredDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	const std::vector<const char*> RequiredValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	#endif
	
	enum RenderBackend {
		OPENGL_BACKEND,
		VULKAN_BACKEND,
		DEKO_BACKEND,
	};

	// the Window class handles our deko3d front/back buffers as well other global-ish datastructres
	// for opengl, we just handle a GLFW window
	class Window {
		// TODO re-enable in eggine repo
		// friend LiteHTMLContainer;
		friend class Shader;
		friend class Program;
		friend VulkanPipeline;
		
		public:
			double deltaTime = 0.0;
			unsigned int width = 1280;
			unsigned int height = 720;

			// TODO re-enable in eggine repo
			// shared_ptr<litehtml::document> htmlDocument = nullptr;
			// render::LiteHTMLContainer* htmlContainer = nullptr;
			
			void initialize(); // start the graphics
			void deinitialize(); // end the graphics
			// TODO re-enable in eggine repo
			// void initializeHTML(); // load index.html
			void resize(unsigned int width, unsigned int height); // resize the window
			void prerender();
			void render();
			void addError();
			unsigned int getErrorCount();
			void clearErrors();
			void registerHTMLUpdate();

			State &getState(uint32_t id); // get a render state, potentially allocate a new one

			#ifdef __switch__
			switch_memory::Manager memory = switch_memory::Manager(this);
			dk::UniqueDevice device;
			dk::CmdBuf commandBuffer;

			HidAnalogStickState leftStick;
			HidAnalogStickState rightStick;
			uint64_t buttons;

			void addTexture(switch_memory::Piece* tempMemory, dk::ImageView& view, unsigned int width, unsigned int height);
			void bindTexture(unsigned int location, class Texture* texture);
			void initializeDeko3d();
			#else
			GLFWwindow* window = nullptr;
			GLFWgamepadstate gamepad;
			bool hasGamepad;

			void initializeOpenGL();
			void initializeVulkan();
			#endif

			// TODO re-enable in eggine repo
			// binds::GamepadButtons axisDPadCounters[4] = {binds::INVALID_BUTTON, binds::INVALID_BUTTON, binds::INVALID_BUTTON, binds::INVALID_BUTTON};

			#ifdef __switch__
			RenderBackend backend = DEKO_BACKEND;
			#else
			RenderBackend backend = VULKAN_BACKEND;
			#endif

		protected:
		public:
			unsigned int errorCount = 0;

			glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

			uint64_t lastRenderTime = getMicrosecondsNow();

			// TODO re-enable in eggine repo
			// litehtml::context htmlContext;
			uint64_t htmlChecksum = 0;
			uint64_t lastHTMLChecksum = 0;

			std::vector<class Program*> programs;

			#ifdef __switch__
			switch_memory::Piece* imageDescriptorMemory;
			switch_memory::Piece* samplerDescriptorMemory;
			
			dk::MemBlock commandBufferMemory;
			unsigned int commandBufferSize = 1024 * 1024; // 1 MB
			unsigned int commandBufferCount = COMMAND_BUFFER_SLICE_COUNT;
			unsigned int commandBufferSliceSize = this->commandBufferSize / COMMAND_BUFFER_SLICE_COUNT;
			unsigned int currentCommandBuffer = 0;
			unsigned int signaledFence = 0;
			dk::Fence commandBufferFences[COMMAND_BUFFER_SLICE_COUNT];
			
			unsigned int staticCommandBufferSize = 16 * 1024; // 16 KB
			dk::MemBlock staticCommandBufferMemory;
			dk::CmdBuf staticCommandBuffer; // always inserted at the start of prerender
			DkCmdList staticCommandList;

			dk::UniqueQueue queue;

			dk::MemBlock framebufferMemory;
			dk::Image framebuffers[2]; // front and back buffer
			DkCmdList framebufferCommandLists[2]; // command lists to bind front and back buffers
			dk::Swapchain swapchain; // handles swapping the front/back buffer during the rendering process

			dk::MemBlock textureCommandBufferMemory;
			dk::CmdBuf textureCommandBuffer;
			dk::Fence textureFence;

			// static data used for building static command list
			DkViewport viewport = { 0.0f, 0.0f, (float)this->width, (float)this->height, 0.0f, 1.0f };
			DkScissor scissor = { 0, 0, this->width, this->height };
			dk::RasterizerState rasterizerState = dk::RasterizerState {};
			dk::ColorState colorState = dk::ColorState {};
			dk::ColorWriteState colorWriteState = dk::ColorWriteState {};
			dk::BlendState blendState = dk::BlendState {};

			PadState pad;
			#else
			tsl::robin_map<uint32_t, State> renderStates;
			
			vk::Instance instance;
			vk::SurfaceKHR surface;
			vk::Queue graphicsQueue;
			vk::Queue presentationQueue;
			Device device;
			vk::DebugUtilsMessengerEXT debugCallback;
			vk::SwapchainKHR swapchain;
			vk::Extent2D swapchainExtent;
			vk::SurfaceFormatKHR swapchainFormat;
			vk::RenderPass renderPass;
			std::vector<vk::Framebuffer> framebuffers;
			uint32_t currentFramebuffer;
			uint32_t framePingPong = 0;

			std::vector<vk::Image> renderImages;
			std::vector<vk::ImageView> renderImageViews;

			tsl::robin_map<VulkanPipeline, VulkanPipelineResult> pipelineCache;

			vk::CommandPool commandPool;

			vk::Fence frameFence[2];
			vk::Semaphore isImageAvailable[2];
			vk::Semaphore isRenderFinished[2];

			void pickDevice();
			void setupDevice();
			#endif
	};
};
