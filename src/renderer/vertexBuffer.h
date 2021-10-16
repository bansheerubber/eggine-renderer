#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#else
#include <GLFW/glfw3.h>
#endif

#include "memory.h"

#define RENDER_UNBIND_VERTEX_BUFFERS

namespace render {
	class VertexBuffer {
		friend class VertexAttributes;
		
		public:
			VertexBuffer(class Window* window);
			~VertexBuffer();
			
			void setData(void* data, unsigned int size, unsigned int align);

			void createBuffer(); // create a normal buffer
			void createDynamicBuffer(); // create buffer with GL_DYNAMIC_DRAW
			void destroyBuffer();

			class Window* window;

			#ifdef __switch__
			void bind(unsigned short id);
			#else
			void bind();
			#endif
		
		protected:
			#ifdef __switch__
			bool memoryAllocated = false;
			switch_memory::Piece* memory = nullptr;
			uint32_t size = 0;
			uint32_t align = 0;
			#else
			GLuint bufferId = GL_INVALID_INDEX;
			GLenum usage = GL_STATIC_DRAW;
			unsigned int size = 0; // current size of the buffer
			#endif
	};
};
