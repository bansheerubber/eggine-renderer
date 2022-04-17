#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#include <switch.h>
#else
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#endif

#include <cstdint>

#include "primitive.h"
#include "stencil.h"

namespace render {
	struct SubState {
		StencilFunction stencilFunction;
		uint32_t stencilReference;
		uint32_t stencilMask;

		uint32_t stencilWriteMask;

		StencilOperation stencilFail;
		StencilOperation depthFail;
		StencilOperation stencilPass;

		bool stencilEnabled;
		bool depthEnabled;

		PrimitiveType primitive;

		class Program* program = nullptr;

		class VertexAttributes* attributes = nullptr;
	};

	inline bool operator==(const SubState &a, const SubState &b) {
		return a.stencilFunction == b.stencilFunction
			&& a.stencilReference == b.stencilReference
			&& a.stencilMask == b.stencilMask
			&& a.stencilWriteMask == b.stencilWriteMask
			&& a.stencilFail == b.stencilFail
			&& a.depthFail == b.depthFail
			&& a.stencilPass == b.stencilPass
			&& a.stencilEnabled == b.stencilEnabled
			&& a.depthEnabled == b.depthEnabled
			&& a.primitive == b.primitive
			&& a.program == b.program
			&& a.attributes == b.attributes;
	}

	inline bool operator!=(const SubState &a, const SubState &b) {
		return !(a == b);
	}
	
	// render state is built around the concept of a command buffer having commands executed in it that influence the
	// effect of following commands. we need to keep track of this state sometimes, particularly for vulkan, where we
	// may have to create/bind a new render pipeline just to switch shaders, which is a very involved process
	class State {
		friend class Window;
		
		public:
			State();
			State(class Window* window);
			
			void draw(PrimitiveType type, unsigned int firstVertex, unsigned int vertexCount, unsigned int firstInstance, unsigned int instanceCount);

			void bindProgram(class Program* program);
			void bindVertexAttributes(class VertexAttributes* attributes);

			void setStencilFunction(StencilFunction func, unsigned int reference, unsigned int mask);
			void setStencilMask(unsigned int mask);
			void setStencilOperation(StencilOperation stencilFail, StencilOperation depthFail, StencilOperation pass);
			void enableStencilTest(bool enable);
			void enableDepthTest(bool enable);

			void reset(); // gets called at the end of the frame
		
		private:
			class Window* window = nullptr;

			vk::CommandBuffer buffer[2];
			
			SubState current;
			SubState old;

			bool applied = false;

			void bindPipeline();
	};
};
