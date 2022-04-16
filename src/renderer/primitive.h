#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#include <switch.h>
#else
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#endif

namespace render {
	enum PrimitiveType {
		PRIMITIVE_POINTS,
		PRIMITIVE_LINES,
		PRIMITIVE_LINE_LOOP,
		PRIMITIVE_LINE_STRIP,
		PRIMITIVE_LINES_ADJACENCY,
		PRIMITIVE_LINE_STRIP_ADJACENCY,
		PRIMITIVE_TRIANGLES,
		PRIMITIVE_TRIANGLE_STRIP,
		PRIMITIVE_TRIANGLE_FAN,
		PRIMITIVE_TRIANGLES_ADJACENCY,
		PRIMITIVE_TRIANGLE_STRIP_ADJACENCY,
		PRIMITIVE_PATCHES,
	};

	#ifdef __switch__
	#define COMMAND_BUFFER_SLICE_COUNT 2
	#define IMAGE_SAMPLER_DESCRIPTOR_COUNT 32

	inline DkPrimitive primitiveToDkPrimitive(PrimitiveType type) {
		switch(type) {
			default:
			case PRIMITIVE_POINTS: {
				return DkPrimitive_Points;
			}

			case PRIMITIVE_LINES: {
				return DkPrimitive_Lines;
			}

			case PRIMITIVE_LINE_LOOP: {
				return DkPrimitive_LineLoop;
			}

			case PRIMITIVE_LINE_STRIP: {
				return DkPrimitive_LineStrip;
			}

			case PRIMITIVE_LINES_ADJACENCY: {
				return DkPrimitive_LinesAdjacency;
			}

			case PRIMITIVE_LINE_STRIP_ADJACENCY: {
				return DkPrimitive_LineStripAdjacency;
			}

			case PRIMITIVE_TRIANGLES: {
				return DkPrimitive_Triangles;
			}

			case PRIMITIVE_TRIANGLE_STRIP: {
				return DkPrimitive_TriangleStrip;
			}

			case PRIMITIVE_TRIANGLE_FAN: {
				return DkPrimitive_TriangleFan;
			}

			case PRIMITIVE_TRIANGLES_ADJACENCY: {
				return DkPrimitive_TrianglesAdjacency;
			}

			case PRIMITIVE_TRIANGLE_STRIP_ADJACENCY: {
				return DkPrimitive_TriangleStripAdjacency;
			}

			case PRIMITIVE_PATCHES: {
				return DkPrimitive_Patches;
			}
		}
	}
	#else
	inline GLenum primitiveToGLPrimitive(PrimitiveType type) {
		switch(type) {
			case PRIMITIVE_POINTS: {
				return GL_POINTS;
			}

			case PRIMITIVE_LINES: {
				return GL_LINES;
			}

			case PRIMITIVE_LINE_LOOP: {
				return GL_LINE_LOOP;
			}

			case PRIMITIVE_LINE_STRIP: {
				return GL_LINE_STRIP;
			}

			case PRIMITIVE_LINES_ADJACENCY: {
				return GL_LINES_ADJACENCY;
			}

			case PRIMITIVE_LINE_STRIP_ADJACENCY: {
				return GL_LINE_STRIP_ADJACENCY;
			}

			case PRIMITIVE_TRIANGLES: {
				return GL_TRIANGLES;
			}

			case PRIMITIVE_TRIANGLE_STRIP: {
				return GL_TRIANGLE_STRIP;
			}

			case PRIMITIVE_TRIANGLE_FAN: {
				return GL_TRIANGLE_FAN;
			}

			case PRIMITIVE_TRIANGLES_ADJACENCY: {
				return GL_TRIANGLES_ADJACENCY;
			}

			case PRIMITIVE_TRIANGLE_STRIP_ADJACENCY: {
				return GL_TRIANGLE_STRIP_ADJACENCY;
			}

			case PRIMITIVE_PATCHES: {
				return GL_PATCHES;
			}

			default: {
				return GL_INVALID_ENUM;
			}
		}
	}

	inline vk::PrimitiveTopology primitiveToVulkanPrimitive(PrimitiveType type) {
		switch(type) {
			case PRIMITIVE_POINTS: {
				return vk::PrimitiveTopology::ePointList;
			}

			case PRIMITIVE_LINES: {
				return vk::PrimitiveTopology::eLineList;
			}

			case PRIMITIVE_LINE_STRIP: {
				return vk::PrimitiveTopology::eLineStrip;
			}

			case PRIMITIVE_LINES_ADJACENCY: {
				return vk::PrimitiveTopology::eLineListWithAdjacency;
			}

			case PRIMITIVE_LINE_STRIP_ADJACENCY: {
				return vk::PrimitiveTopology::eLineStripWithAdjacency;
			}

			case PRIMITIVE_TRIANGLES: {
				return vk::PrimitiveTopology::eTriangleList;
			}

			case PRIMITIVE_TRIANGLE_STRIP: {
				return vk::PrimitiveTopology::eTriangleStrip;
			}

			case PRIMITIVE_TRIANGLE_FAN: {
				return vk::PrimitiveTopology::eTriangleFan;
			}

			case PRIMITIVE_TRIANGLES_ADJACENCY: {
				return vk::PrimitiveTopology::eTriangleListWithAdjacency;
			}

			case PRIMITIVE_TRIANGLE_STRIP_ADJACENCY: {
				return vk::PrimitiveTopology::eTriangleStripWithAdjacency;
			}

			case PRIMITIVE_PATCHES: {
				return vk::PrimitiveTopology::ePatchList;
			}

			default: {
				return vk::PrimitiveTopology::ePointList;
			}
		}
	}
	#endif
};
