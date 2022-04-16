#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#include <switch.h>
#else
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#endif

#include "../util/hash.h"
#include "primitive.h"

namespace render {
	struct VulkanPipelineResult {
		vk::PipelineLayout* layout;
		vk::Pipeline* pipeline;
	};
	
	struct VulkanPipeline { // used for caching a vk pipeline based on commonly used parameters
		class Window* window;
		PrimitiveType topology;
		float viewportWidth;
		float viewportHeight;
		class Program* program;

		VulkanPipelineResult newPipeline();
	};

	inline bool operator==(const VulkanPipeline &lhs, const VulkanPipeline &rhs) {
		return lhs.topology == rhs.topology && lhs.viewportWidth == rhs.viewportWidth && lhs.viewportHeight == rhs.viewportHeight && lhs.program == rhs.program;
	}
};

namespace std {
	template<>
	struct hash<render::VulkanPipeline> {
		size_t operator()(const render::VulkanPipeline &source) const noexcept {
			uint64_t result = hash<render::PrimitiveType>{}(source.topology);
			result = combineHash(result, source.viewportWidth);
			result = combineHash(result, source.viewportHeight);
			result = combineHash(result, source.program);
			return result;
    }
	};
};
