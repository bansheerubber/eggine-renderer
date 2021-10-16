#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#endif

#include <string>

#include "memory.h"

using namespace std;

namespace render {
	class Shader {
		public:
			Shader(class Window* window);

			void load(string filename);
			void bind();

			dk::Shader shader;

		protected:
			class Window* window;

			#ifdef __switch__
			switch_memory::Piece* memory = nullptr;
			#endif
	};
};
