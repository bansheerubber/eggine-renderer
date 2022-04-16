#pragma once

#include <string>

#include "shaderSource.h"

namespace render {
	class Window;
};

resources::ShaderSource* getShaderSource(render::Window* window, std::string fileName);
