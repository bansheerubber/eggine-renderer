#pragma once

#ifndef __switch__
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#endif

#include <array>
#include <tsl/robin_map.h>
#include <string>
#include <vector>

#include "pipeline.h"
#include "memory.h"

namespace std {
	template<>
	struct hash<pair<std::string, uint64_t>> {
		size_t operator()(pair<std::string, uint64_t> const& source) const noexcept {
			uint64_t result = hash<std::string>{}(source.first);
			return result ^ (source.second + 0x9e3779b9 + (result << 6) + (result >> 2));
    }
	};

	template<>
	struct equal_to<pair<std::string, uint64_t>> {
		bool operator()(const pair<std::string, uint64_t>& x, const pair<std::string, uint64_t>& y) const {
			return x.first == y.first && x.second == y.second;
		}
	};
};

namespace render {
	class Program {
		#ifndef __switch__
		friend VulkanPipeline;
		#endif
		friend class State;
		friend class Window;

		public:
			Program(class Window* window);
			void compile();
			void addShader(class Shader* shader);
			void bindUniform(std::string uniformNamde, void* data, unsigned int size, uint64_t cacheIndex = 0, bool setOnce = false);
			void bindTexture(std::string uniformName, unsigned int texture);
		
		protected:
			std::vector<class Shader*> shaders;
			Window* window = nullptr;

			tsl::robin_map<std::string, unsigned int> uniformToBinding;

			#ifdef __switch__
			tsl::robin_map<std::string, switch_memory::Piece*> uniformToPiece;
			void createUniformMemory(std::string uniformName, unsigned int size);
			#else
			GLuint program = GL_INVALID_INDEX;
			tsl::robin_map<std::pair<std::string, uint64_t>, GLuint> uniformToBuffer;

			std::vector<vk::PipelineShaderStageCreateInfo> stages = std::vector<vk::PipelineShaderStageCreateInfo>(2);
			uint8_t stageCount = 0;

			static unsigned int UniformCount;
			void createUniformBuffer(std::string uniformName, unsigned int size, uint64_t cacheIndex);
			#endif
	};
};
