#pragma once

#ifndef __switch__
#include <GLFW/glfw3.h>
#endif

#include <array>
#include <vector>

using namespace std;

namespace render {
	class Program {
		public:
			Program(class Window* window);
			void bind();
			void addShader(class Shader* shader);
		
		protected:
			vector<class Shader*> shaders;
			Window* window = nullptr;

			#ifndef __switch__
			GLuint program = GL_INVALID_INDEX;
			#endif
	};
};
