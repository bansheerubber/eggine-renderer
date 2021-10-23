#pragma once

#ifndef __switch__
#include <GLFW/glfw3.h>
#endif

#include <array>
#include <tsl/robin_map.h>
#include <vector>

#include "memory.h"

using namespace std;

namespace render {
	class Program {
		public:
			Program(class Window* window);
			void bind();
			void addShader(class Shader* shader);
			void bindUniform(string uniformName, void* data, unsigned int size);
			void bindTexture(string uniformName, unsigned int texture);
		
		protected:
			vector<class Shader*> shaders;
			Window* window = nullptr;

			tsl::robin_map<string, unsigned int> uniformToBinding;

			#ifdef __switch__
			tsl::robin_map<string, switch_memory::Piece*> uniformToPiece;
			void createUniformMemory(string uniformName, unsigned int size);
			#else
			GLuint program = GL_INVALID_INDEX;
			tsl::robin_map<string, GLuint> uniformToBuffer;
			void createUniformBuffer(string uniformName, unsigned int size);
			#endif
	};
};
