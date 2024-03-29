#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#else
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#endif

#include "memory.h"

#define RENDER_UNBIND_VERTEX_BUFFERS

namespace render {
	class VertexBuffer {
		friend class State;
		friend class VertexAttributes;
		
		public:
			VertexBuffer(class Window* window);
			~VertexBuffer();
			
			void setDynamicDraw(bool isDynamicDraw);
			void setData(void* data, unsigned int size, unsigned int align);

			class Window* window;

			#ifdef __switch__
			void bind(unsigned short id);
			#else
			void bind();
			#endif
		
		protected:
			#ifdef __switch__
			bool memoryAllocated = false;
			Piece* memory = nullptr;
			uint32_t size = 0;
			uint32_t align = 0;
			#else
			GLuint bufferId = GL_INVALID_INDEX;
			GLenum usage = GL_STATIC_DRAW;
			uint32_t size = 0; // current size of the buffer

			Piece* stagingBuffer = nullptr;
			Piece* gpuBuffer = nullptr;

			bool isDynamicDraw = false;

			bool needsCopy = false;
			#endif
			uint32_t oldSize = 0;

			void destroyBuffer();
	};
};
