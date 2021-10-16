#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#else
#include <GLFW/glfw3.h>
#endif

#include <vector>

#include "memory.h"

using namespace std;

namespace render {
	struct VertexAttribute {
		class VertexBuffer* buffer;
		unsigned short attributeLocation;
		unsigned short vectorLength;
		unsigned short dataSize;
		unsigned short offset;
		unsigned short stride;
		unsigned short divisor;
	};
	
	class VertexAttributes {
		public:
			VertexAttributes(class Window* window);

			void addVertexAttribute(class VertexBuffer* buffer, unsigned short attributeLocation, unsigned short vectorLength, unsigned short dataSize, unsigned short offset, unsigned short stride, unsigned short divisor);
			void bind();

		protected:
			Window* window = nullptr;

			vector<VertexAttribute> attributes;

			#ifdef __switch__
			vector<VertexBuffer*> bufferBindOrder;
			vector<DkVtxAttribState> attributeStates;
			vector<DkVtxBufferState> bufferStates;
			#endif

			void buildCommandLists();
	};
};
