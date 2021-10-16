#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#else
#include <GLFW/glfw3.h>
#endif

#include <string>

#include "memory.h"

using namespace std;

namespace render {
	enum ShaderType {
		SHADER_FRAGMENT,
		SHADER_VERTEX
	};

	class Shader {
		friend class Program;
		
		public:
			Shader(class Window* window);

			void loadFromFile(string filename, ShaderType type);
			void load(char* buffer, size_t length, ShaderType type);
			void bind();

		protected:
			class Window* window;

			#ifdef __switch__
			switch_memory::Piece* memory = nullptr;
			dk::Shader shader;
			#else
			GLuint shader = GL_INVALID_INDEX;
			#endif
	};
};
