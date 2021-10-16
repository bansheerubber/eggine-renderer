#ifdef __switch__
#include <deko3d.hpp>
#include <switch.h>
#else
#include <GLFW/glfw3.h>
#endif

#include <glm/vec4.hpp>

#include "memory.h"
#include "../util/time.h"

namespace render {
	// the Window class handles our deko3d front/back buffers as well other global-ish datastructres
	// for opengl, we just handle a GLFW window
	class Window {
		public:
			void initialize(); // start the graphics
			void deinitialize(); // end the graphics
			void resize(unsigned int width, unsigned int height); // resize the window
			void prerender();
			void render();

			double deltaTime = 0.0;

			#ifdef __switch__
			switch_memory::Manager memory = switch_memory::Manager(this);
			dk::UniqueDevice device;
			dk::CmdBuf commandBuffer;
			DkCmdList commandList;
			#endif

		protected:
			unsigned int width = 1280;
			unsigned int height = 720;

			glm::vec4 clearColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

			unsigned long long lastRenderTime = getMicrosecondsNow();
			
			#ifdef __switch__
			unsigned int nxlink = 0;
			unsigned int commandBufferSize = 16 * 1024; // 16 KB
			
			dk::UniqueQueue queue;

			dk::MemBlock framebufferMemory;
			dk::Image framebuffers[2]; // front and back buffer
			DkCmdList framebufferCommandLists[2]; // command lists to bind front and back buffers
			dk::Swapchain swapchain; // handles swapping the front/back buffer during the rendering process

			dk::MemBlock commandBufferMemory;

			dk::MemBlock staticCommandBufferMemory;
			dk::CmdBuf staticCommandBuffer; // always inserted at the start of prerender
			DkCmdList staticCommandList;

			// static data used for building static command list
			DkViewport viewport = { 0.0f, 0.0f, (float)this->width, (float)this->height, 0.0f, 1.0f };
			DkScissor scissor = { 0, 0, this->width, this->height };
			dk::RasterizerState rasterizerState = dk::RasterizerState {};
			dk::ColorState colorState = dk::ColorState {};
			dk::ColorWriteState colorWriteState = dk::ColorWriteState {};
			#else
			GLFWwindow* window = nullptr;
			#endif
	};
};
