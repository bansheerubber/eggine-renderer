#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#else
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#endif

#include <vector>

#include "memory.h"

namespace render {
	enum VertexAttributeType {
		VERTEX_ATTRIB_BYTE,
		VERTEX_ATTRIB_UNSIGNED_BYTE,
		VERTEX_ATTRIB_SHORT,
		VERTEX_ATTRIB_UNSIGNED_SHORT,
		VERTEX_ATTRIB_INT,
		VERTEX_ATTRIB_UNSIGNED_INT,
		VERTEX_ATTRIB_HALF_FLOAT,
		VERTEX_ATTRIB_FLOAT,
		VERTEX_ATTRIB_DOUBLE,
	};
	
	struct VertexAttribute {
		class VertexBuffer* buffer;
		unsigned short attributeLocation;
		unsigned short vectorLength;
		VertexAttributeType type;
		unsigned short offset;
		unsigned short stride;
		unsigned short divisor;
	};

	inline unsigned int attributeTypeToSize(VertexAttributeType type) {
		switch(type) {
			case VERTEX_ATTRIB_UNSIGNED_BYTE:
			case VERTEX_ATTRIB_BYTE: {
				return 1;
			}

			case VERTEX_ATTRIB_SHORT:
			case VERTEX_ATTRIB_UNSIGNED_SHORT: {
				return 2;
			}

			case VERTEX_ATTRIB_INT:
			case VERTEX_ATTRIB_UNSIGNED_INT:
			case VERTEX_ATTRIB_FLOAT: {
				return 4;
			}

			case VERTEX_ATTRIB_DOUBLE: {
				return 8;
			}

			default: {
				return 0;
			}
		}
	}

	#ifdef __switch__
	inline DkVtxAttribSize attributeTypeToDkAttribSize(VertexAttributeType type, unsigned int vectorLength) {
		if(vectorLength == 1) {
			switch(type) {
				case VERTEX_ATTRIB_UNSIGNED_BYTE:
				case VERTEX_ATTRIB_BYTE: {
					return DkVtxAttribSize_1x8;
				}

				case VERTEX_ATTRIB_SHORT:
				case VERTEX_ATTRIB_UNSIGNED_SHORT: {
					return DkVtxAttribSize_1x16;
				}

				case VERTEX_ATTRIB_INT:
				case VERTEX_ATTRIB_UNSIGNED_INT:
				case VERTEX_ATTRIB_FLOAT: {
					return DkVtxAttribSize_1x32;
				}
			}
		}
		else if(vectorLength == 2) {
			switch(type) {
				case VERTEX_ATTRIB_UNSIGNED_BYTE:
				case VERTEX_ATTRIB_BYTE: {
					return DkVtxAttribSize_2x8;
				}

				case VERTEX_ATTRIB_SHORT:
				case VERTEX_ATTRIB_UNSIGNED_SHORT: {
					return DkVtxAttribSize_2x16;
				}

				case VERTEX_ATTRIB_INT:
				case VERTEX_ATTRIB_UNSIGNED_INT:
				case VERTEX_ATTRIB_FLOAT: {
					return DkVtxAttribSize_2x32;
				}
			}
		}
		else if(vectorLength == 3) {
			switch(type) {
				case VERTEX_ATTRIB_UNSIGNED_BYTE:
				case VERTEX_ATTRIB_BYTE: {
					return DkVtxAttribSize_3x8;
				}

				case VERTEX_ATTRIB_SHORT:
				case VERTEX_ATTRIB_UNSIGNED_SHORT: {
					return DkVtxAttribSize_3x16;
				}

				case VERTEX_ATTRIB_INT:
				case VERTEX_ATTRIB_UNSIGNED_INT:
				case VERTEX_ATTRIB_FLOAT: {
					return DkVtxAttribSize_3x32;
				}
			}
		}
		else if(vectorLength == 4) {
			switch(type) {
				case VERTEX_ATTRIB_UNSIGNED_BYTE:
				case VERTEX_ATTRIB_BYTE: {
					return DkVtxAttribSize_4x8;
				}

				case VERTEX_ATTRIB_SHORT:
				case VERTEX_ATTRIB_UNSIGNED_SHORT: {
					return DkVtxAttribSize_4x16;
				}

				case VERTEX_ATTRIB_INT:
				case VERTEX_ATTRIB_UNSIGNED_INT:
				case VERTEX_ATTRIB_FLOAT: {
					return DkVtxAttribSize_4x32;
				}
			}
		}

		return DkVtxAttribSize_1x8;
	}

	inline DkVtxAttribType attributeTypeToDkAttribType(VertexAttributeType type) {
		switch(type) {
			default:
			case VERTEX_ATTRIB_UNSIGNED_BYTE:
			case VERTEX_ATTRIB_UNSIGNED_SHORT:
			case VERTEX_ATTRIB_UNSIGNED_INT: {
				return DkVtxAttribType_Uint;
			}

			case VERTEX_ATTRIB_BYTE:
			case VERTEX_ATTRIB_SHORT:
			case VERTEX_ATTRIB_INT: {
				return DkVtxAttribType_Sint;
			}

			case VERTEX_ATTRIB_FLOAT: {
				return DkVtxAttribType_Float;
			}
		}
	}
	#else
	inline GLenum attributeTypeToGLType(VertexAttributeType type) {
		switch(type) {
			case VERTEX_ATTRIB_UNSIGNED_BYTE: {
				return GL_UNSIGNED_BYTE;
			}

			case VERTEX_ATTRIB_BYTE: {
				return GL_BYTE;
			}

			case VERTEX_ATTRIB_SHORT: {
				return GL_SHORT;
			}

			case VERTEX_ATTRIB_UNSIGNED_SHORT: {
				return GL_UNSIGNED_SHORT;
			}

			case VERTEX_ATTRIB_INT: {
				return GL_INT;
			}

			case VERTEX_ATTRIB_UNSIGNED_INT: {
				return GL_UNSIGNED_INT;
			}

			case VERTEX_ATTRIB_HALF_FLOAT: {
				return GL_HALF_FLOAT;
			}

			case VERTEX_ATTRIB_FLOAT: {
				return GL_FLOAT;
			}

			case VERTEX_ATTRIB_DOUBLE: {
				return GL_DOUBLE;
			}

			default: {
				return GL_INVALID_ENUM;
			}
		}
	}

	inline vk::Format attributeTypeToVulkanType(VertexAttributeType type, unsigned short vectorLength) {
		if(vectorLength == 1) {
			switch(type) {
				case VERTEX_ATTRIB_UNSIGNED_BYTE: {
					return vk::Format::eR8Uint;
				}

				case VERTEX_ATTRIB_BYTE: {
					return vk::Format::eR8Sint;
				}

				case VERTEX_ATTRIB_UNSIGNED_SHORT: {
					return vk::Format::eR16Uint;
				}

				case VERTEX_ATTRIB_SHORT: {
					return vk::Format::eR16Sint;
				}

				case VERTEX_ATTRIB_UNSIGNED_INT: {
					return vk::Format::eR32Uint;
				}

				case VERTEX_ATTRIB_INT: {
					return vk::Format::eR32Sint;
				}

				case VERTEX_ATTRIB_FLOAT: {
					return vk::Format::eR32Sfloat;
				}
			}
		}
		else if(vectorLength == 2) {
			switch(type) {
				case VERTEX_ATTRIB_UNSIGNED_BYTE: {
					return vk::Format::eR8G8Uint;
				}

				case VERTEX_ATTRIB_BYTE: {
					return vk::Format::eR8G8Sint;
				}

				case VERTEX_ATTRIB_UNSIGNED_SHORT: {
					return vk::Format::eR16G16Uint;
				}

				case VERTEX_ATTRIB_SHORT: {
					return vk::Format::eR16G16Sint;
				}

				case VERTEX_ATTRIB_UNSIGNED_INT: {
					return vk::Format::eR32G32Uint;
				}

				case VERTEX_ATTRIB_INT: {
					return vk::Format::eR32G32Sint;
				}

				case VERTEX_ATTRIB_FLOAT: {
					return vk::Format::eR32G32Sfloat;
				}
			}
		}
		else if(vectorLength == 3) {
			switch(type) {
				case VERTEX_ATTRIB_UNSIGNED_BYTE: {
					return vk::Format::eR8G8B8Uint;
				}

				case VERTEX_ATTRIB_BYTE: {
					return vk::Format::eR8G8B8Sint;
				}

				case VERTEX_ATTRIB_UNSIGNED_SHORT: {
					return vk::Format::eR16G16B16Uint;
				}

				case VERTEX_ATTRIB_SHORT: {
					return vk::Format::eR16G16B16Sint;
				}

				case VERTEX_ATTRIB_UNSIGNED_INT: {
					return vk::Format::eR32G32B32Uint;
				}

				case VERTEX_ATTRIB_INT: {
					return vk::Format::eR32G32B32Sint;
				}

				case VERTEX_ATTRIB_FLOAT: {
					return vk::Format::eR32G32B32Sfloat;
				}
			}
		}
		else if(vectorLength == 4) {
			switch(type) {
				case VERTEX_ATTRIB_UNSIGNED_BYTE: {
					return vk::Format::eR8G8B8A8Uint;
				}

				case VERTEX_ATTRIB_BYTE: {
					return vk::Format::eR8G8B8A8Sint;
				}

				case VERTEX_ATTRIB_UNSIGNED_SHORT: {
					return vk::Format::eR16G16B16A16Uint;
				}

				case VERTEX_ATTRIB_SHORT: {
					return vk::Format::eR16G16B16A16Sint;
				}

				case VERTEX_ATTRIB_UNSIGNED_INT: {
					return vk::Format::eR32G32B32A32Uint;
				}

				case VERTEX_ATTRIB_INT: {
					return vk::Format::eR32G32B32A32Sint;
				}

				case VERTEX_ATTRIB_FLOAT: {
					return vk::Format::eR32G32B32A32Sfloat;
				}
			}
		}

		return vk::Format::eR8Uint;
	}
	#endif
	
	class VertexAttributes {
		friend struct VulkanPipeline;
		friend class State;
		
		public:
			VertexAttributes(class Window* window);

			void addVertexAttribute(class VertexBuffer* buffer, unsigned short attributeLocation, unsigned short vectorLength, VertexAttributeType type, unsigned short offset, unsigned short stride, unsigned short divisor);

		protected:
			Window* window = nullptr;

			std::vector<VertexAttribute> attributes;

			#ifdef __switch__
			std::vector<VertexBuffer*> bufferBindOrder;
			std::vector<DkVtxAttribState> attributeStates;
			std::vector<DkVtxBufferState> bufferStates;
			#else
			GLuint vertexArrayObject = GL_INVALID_INDEX;

			std::vector<vk::VertexInputBindingDescription> inputBindings;
			std::vector<vk::VertexInputAttributeDescription> inputAttributes;
			#endif

			void buildCommandLists();
	};
};
