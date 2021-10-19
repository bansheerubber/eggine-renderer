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
	enum PrimitiveType {
		PRIMITIVE_POINTS,
		PRIMITIVE_LINES,
		PRIMITIVE_LINE_LOOP,
		PRIMITIVE_LINE_STRIP,
		PRIMITIVE_LINES_ADJACENCY,
		PRIMITIVE_LINE_STRIP_ADJACENCY,
		PRIMITIVE_TRIANGLES,
		PRIMITIVE_TRIANGLE_STRIP,
		PRIMITIVE_TRIANGLE_FAN,
		PRIMITIVE_TRIANGLES_ADJACENCY,
		PRIMITIVE_TRIANGLE_STRIP_ADJACENCY,
		PRIMITIVE_PATCHES,
	};

	#ifdef __switch__
	#define COMMAND_BUFFER_SLICE_COUNT 2
	#define IMAGE_SAMPLER_DESCRIPTOR_COUNT 32

	inline DkPrimitive primitiveToDkPrimitive(PrimitiveType type) {
		switch(type) {
			case PRIMITIVE_POINTS: {
				return DkPrimitive_Points;
			}

			case PRIMITIVE_LINES: {
				return DkPrimitive_Lines;
			}

			case PRIMITIVE_LINE_LOOP: {
				return DkPrimitive_LineLoop;
			}

			case PRIMITIVE_LINE_STRIP: {
				return DkPrimitive_LineStrip;
			}

			case PRIMITIVE_LINES_ADJACENCY: {
				return DkPrimitive_LinesAdjacency;
			}

			case PRIMITIVE_LINE_STRIP_ADJACENCY: {
				return DkPrimitive_LineStripAdjacency;
			}

			case PRIMITIVE_TRIANGLES: {
				return DkPrimitive_Triangles;
			}

			case PRIMITIVE_TRIANGLE_STRIP: {
				return DkPrimitive_TriangleStrip;
			}

			case PRIMITIVE_TRIANGLE_FAN: {
				return DkPrimitive_TriangleFan;
			}

			case PRIMITIVE_TRIANGLES_ADJACENCY: {
				return DkPrimitive_TrianglesAdjacency;
			}

			case PRIMITIVE_TRIANGLE_STRIP_ADJACENCY: {
				return DkPrimitive_TriangleStripAdjacency;
			}

			case PRIMITIVE_PATCHES: {
				return DkPrimitive_Patches;
			}
		}
	}
	#else
	inline GLenum primitiveToGLPrimitive(PrimitiveType type) {
		switch(type) {
			case PRIMITIVE_POINTS: {
				return GL_POINTS;
			}

			case PRIMITIVE_LINES: {
				return GL_LINES;
			}

			case PRIMITIVE_LINE_LOOP: {
				return GL_LINE_LOOP;
			}

			case PRIMITIVE_LINE_STRIP: {
				return GL_LINE_STRIP;
			}

			case PRIMITIVE_LINES_ADJACENCY: {
				return GL_LINES_ADJACENCY;
			}

			case PRIMITIVE_LINE_STRIP_ADJACENCY: {
				return GL_LINE_STRIP_ADJACENCY;
			}

			case PRIMITIVE_TRIANGLES: {
				return GL_TRIANGLES;
			}

			case PRIMITIVE_TRIANGLE_STRIP: {
				return GL_TRIANGLE_STRIP;
			}

			case PRIMITIVE_TRIANGLE_FAN: {
				return GL_TRIANGLE_FAN;
			}

			case PRIMITIVE_TRIANGLES_ADJACENCY: {
				return GL_TRIANGLES_ADJACENCY;
			}

			case PRIMITIVE_TRIANGLE_STRIP_ADJACENCY: {
				return GL_TRIANGLE_STRIP_ADJACENCY;
			}

			case PRIMITIVE_PATCHES: {
				return GL_PATCHES;
			}
		}
	}
	#endif

	// the Window class handles our deko3d front/back buffers as well other global-ish datastructres
	// for opengl, we just handle a GLFW window
	class Window {
		public:
			double deltaTime = 0.0;
			
			void initialize(); // start the graphics
			void deinitialize(); // end the graphics
			void resize(unsigned int width, unsigned int height); // resize the window
			void prerender();
			void render();
			void draw(PrimitiveType type, unsigned int firstVertex, unsigned int vertexCount, unsigned int firstInstance, unsigned int instanceCount);
			void addError();
			unsigned int getErrorCount();
			void clearErrors();

			#ifdef __switch__
			switch_memory::Manager memory = switch_memory::Manager(this);
			dk::UniqueDevice device;
			dk::CmdBuf commandBuffer;

			void addTexture(switch_memory::Piece* tempMemory, dk::ImageView view, unsigned int width, unsigned int height);
			void bindTexture(unsigned int location, class Texture* texture);
			#endif

		protected:
			unsigned int errorCount = 0;
			unsigned int width = 1280;
			unsigned int height = 720;

			glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

			unsigned long long lastRenderTime = getMicrosecondsNow();
			
			#ifdef __switch__
			unsigned int nxlink = 0;

			switch_memory::Piece* imageDescriptorMemory;
			switch_memory::Piece* samplerDescriptorMemory;
			
			dk::MemBlock commandBufferMemory;
			unsigned int commandBufferSize = 1024 * 1024; // 1 MB
			unsigned int commandBufferCount = COMMAND_BUFFER_SLICE_COUNT;
			unsigned int commandBufferSliceSize = this->commandBufferSize / COMMAND_BUFFER_SLICE_COUNT;
			unsigned int currentCommandBuffer = 0;
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
			#else
			GLFWwindow* window = nullptr;
			#endif
	};
};
