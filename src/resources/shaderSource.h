#pragma once

#include <string>

#include "../engine/console.h"

namespace resources {
	class ShaderSource {
		public:
			ShaderSource(const unsigned char* buffer, uint64_t bufferSize);
			unsigned char* buffer;
			uint64_t bufferSize = 0;
			ShaderSource* original = nullptr;
	};
};
