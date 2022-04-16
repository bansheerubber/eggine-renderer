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
#include "texture.h"
#include "../util/time.h"

namespace render {
	enum StencilFunction {
		STENCIL_NEVER,
		STENCIL_LESS,
		STENCIL_LESS_EQUAL,
		STENCIL_GREATER,
		STENCIL_GREATER_EQUAL,
		STENCIL_EQUAL,
		STENCIL_NOT_EQUAL,
		STENCIL_ALWAYS,
	};

	#ifdef __switch
	#else
	inline GLenum stencilToGLStencil(StencilFunction type) {
		switch(type) {
			case STENCIL_NEVER: {
				return GL_NEVER;
			}

			case STENCIL_LESS: {
				return GL_LESS;
			}

			case STENCIL_LESS_EQUAL: {
				return GL_LEQUAL;
			}

			case STENCIL_GREATER: {
				return GL_GREATER;
			}

			case STENCIL_GREATER_EQUAL: {
				return GL_GEQUAL;
			}

			case STENCIL_EQUAL: {
				return GL_EQUAL;
			}

			case STENCIL_NOT_EQUAL: {
				return GL_NOTEQUAL;
			}

			case STENCIL_ALWAYS: {
				return GL_ALWAYS;
			}

			default: {
				return GL_NEVER;
			}
		}
	}
	#endif

	enum StencilOperation {
		STENCIL_KEEP,
		STENCIL_ZERO,
		STENCIL_REPLACE,
		STENCIL_INCREMENT,
		STENCIL_INCREMENT_WRAP,
		STENCIL_DECREMENT,
		STENCIL_DECREMENT_WRAP,
		STENCIL_INVERT,
	};

	#ifdef __switch__
	#else
	inline GLenum stencilOPToGLStencilOP(StencilOperation type) {
		switch(type) {
			case STENCIL_KEEP: {
				return GL_KEEP;
			}

			case STENCIL_ZERO: {
				return GL_ZERO;
			}

			case STENCIL_REPLACE: {
				return GL_REPLACE;
			}

			case STENCIL_INCREMENT: {
				return GL_INCR;
			}

			case STENCIL_INCREMENT_WRAP: {
				return GL_INCR_WRAP;
			}

			case STENCIL_DECREMENT: {
				return GL_DECR;
			}

			case STENCIL_DECREMENT_WRAP: {
				return GL_DECR_WRAP;
			}

			case STENCIL_INVERT: {
				return GL_INVERT;
			}

			default: {
				return GL_KEEP;
			}
		}
	}
	#endif

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
			void draw(PrimitiveType type, unsigned int firstVertex, unsigned int vertexCount, unsigned int firstInstance, unsigned int instanceCount);
			void addError();
			unsigned int getErrorCount();
			void clearErrors();
			void registerHTMLUpdate();

			void setStencilFunction(StencilFunction func, unsigned int reference, unsigned int mask);
			void setStencilMask(unsigned int mask);
			void setStencilOperation(StencilOperation stencilFail, StencilOperation depthFail, StencilOperation pass);
			void enableStencilTest(bool enable);

			void enableDepthTest(bool enable);

			#ifdef __switch__
			switch_memory::Manager memory = switch_memory::Manager(this);
			dk::UniqueDevice device;
			dk::CmdBuf commandBuffer;

			HidAnalogStickState leftStick;
			HidAnalogStickState rightStick;
			uint64_t buttons;

			void addTexture(switch_memory::Piece* tempMemory, dk::ImageView& view, unsigned int width, unsigned int height);
			void bindTexture(unsigned int location, class Texture* texture);
			#else
			GLFWwindow* window = nullptr;
			GLFWgamepadstate gamepad;
			bool hasGamepad;
			#endif

			// TODO re-enable in eggine repo
			// binds::GamepadButtons axisDPadCounters[4] = {binds::INVALID_BUTTON, binds::INVALID_BUTTON, binds::INVALID_BUTTON, binds::INVALID_BUTTON};

			#ifdef __switch__
			RenderBackend backend = DEKO_BACKEND;
			#else
			RenderBackend backend = OPENGL_BACKEND;
			#endif

		protected:
			unsigned int errorCount = 0;

			glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

			uint64_t lastRenderTime = getMicrosecondsNow();

			// TODO re-enable in eggine repo
			// litehtml::context htmlContext;
			uint64_t htmlChecksum = 0;
			uint64_t lastHTMLChecksum = 0;
			
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
			vk::Instance instance;
			vk::SurfaceKHR surface;
			vk::Queue graphicsQueue;
			vk::Queue presentationQueue;
			Device device;
			vk::DebugUtilsMessengerEXT debugCallback;
			vk::SwapchainKHR swapchain;
			vk::Extent2D swapchainExtent;
			vk::SurfaceFormatKHR swapchainFormat;

			std::vector<vk::Image> renderImages;
			std::vector<vk::ImageView> renderImageViews;

			tsl::robin_map<VulkanPipeline, VulkanPipelineResult> pipelineCache;
			tsl::robin_map<VulkanPipeline, uint32_t> currentFramebuffer;

			vk::CommandPool commandPool;
			vk::CommandBuffer mainBuffer;

			vk::Fence frameFence;
			vk::Semaphore isImageAvailable;
			vk::Semaphore isRenderFinished;

			Program* simpleProgram = nullptr;
			
			void pickDevice();
			void setupDevice();
			#endif
	};
};
