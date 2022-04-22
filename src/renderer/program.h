#pragma once

#ifndef __switch__
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#endif

#include <array>
#include <tsl/robin_map.h>
#include <string>
#include <vector>

#include "memory.h"
#include "vulkanPipeline.h"

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
			void bindTexture(std::string uniformName, unsigned int texture);
			void bindTexture(std::string uniformName, class Texture* texture);
		
		protected:
			std::vector<class Shader*> shaders;
			Window* window = nullptr;

			tsl::robin_map<std::string, unsigned int> uniformToBinding;

			#ifdef __switch__
			tsl::robin_map<std::string, Piece*> uniformToPiece;
			#else
			GLuint program = GL_INVALID_INDEX;
			tsl::robin_map<std::string, GLuint> uniformToBuffer;
			tsl::robin_map<std::string, Piece*> uniformToVulkanBuffer;
			tsl::robin_map<std::string, uint32_t> uniformToShaderBinding;
			tsl::robin_map<std::string, bool> isUniformSampler;
			tsl::robin_map<std::string, class Texture*> uniformToTexture;

			std::vector<vk::PipelineShaderStageCreateInfo> stages = std::vector<vk::PipelineShaderStageCreateInfo>(2);
			uint8_t stageCount = 0;
			vk::DescriptorSetLayout descriptorLayout;
			vk::DescriptorSet descriptorSet;
			bool descriptorSetInitialized = false;

			bool compiled = false;

			static unsigned int UniformCount;

			void createDescriptorSet();
			#endif

			void createUniformBuffer(std::string uniformName, unsigned int size);
	};
};
