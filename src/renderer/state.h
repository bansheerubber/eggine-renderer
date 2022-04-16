#pragma once

#include <cstdint>

#include "stencil.h"

namespace render {
	// render state is built around the concept of a command buffer having commands executed in it that influence the
	// effect of following commands. we need to keep track of this state sometimes, particularly for vulkan, where we
	// may have to create/bind a new render pipeline just to switch shaders, which is a very involved process
	class State {
		public:
			void bindProgram(class Program* program);
			void draw();
			void setStencilFunction(StencilFunction func, unsigned int reference, unsigned int mask);
			void setStencilMask(unsigned int mask);
			void setStencilOperation(StencilOperation stencilFail, StencilOperation depthFail, StencilOperation pass);
			void enableStencilTest(bool enable);
			void enableDepthTest(bool enable);
		
		private:
			StencilFunction stencilFunction;
			uint32_t stencilReference;
			uint32_t stencilMask;

			uint32_t stencilWriteMask;

			StencilOperation stencilFail;
			StencilOperation depthFail;
			StencilOperation stencilPass;

			bool stencilEnabled;
			bool depthEnabled;

			Program* program;
	};
};
