#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#else
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#endif

#include <tsl/robin_map.h>
#include "../resources/shaderSource.h"
#include <string>

#include "memory.h"

namespace render {
	enum ShaderType {
		SHADER_FRAGMENT,
		SHADER_VERTEX
	};

	class Shader {
		friend class Program;
		friend class State;
		friend class Window;
		
		public:
			Shader(class Window* window);

			void load(resources::ShaderSource* source, ShaderType type);
			void bind();

		protected:
			class Window* window;

			tsl::robin_map<std::string, unsigned int> uniformToBinding;
			ShaderType type;

			#ifdef __switch__
			switch_memory::Piece* memory = nullptr;
			dk::Shader shader;
			#else
			GLuint shader = GL_INVALID_INDEX;
			vk::ShaderModule module;
			vk::PipelineShaderStageCreateInfo stage;
			#endif

			void processUniforms(const char* buffer, uint64_t bufferSize);
	};
};
