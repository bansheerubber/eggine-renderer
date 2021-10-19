#pragma once

#if __switch__
#include <deko3d.hpp>
#include "memory.h"
#else
#include <GLFW/glfw3.h>
#endif

#include <png.h>
#include <string>

using namespace std;

namespace render {
	enum TextureFilter {
		TEXTURE_FILTER_INVALID,
		TEXTURE_FILTER_NEAREST,
		TEXTURE_FILTER_LINEAR,
	};

	enum TextureWrap {
		TEXTURE_WRAP_INVALID,
		TEXTURE_WRAP_REPEAT,
		TEXTURE_WRAP_MIRRORED_REPEAT,
		TEXTURE_WRAP_CLAMP_TO_EDGE,
		TEXTURE_WRAP_CLAMP_TO_BORDER,
		TEXTURE_WRAP_MIRROR_CLAMP_TO_EDGE,
	};

	#ifdef __switch__
	inline DkFilter textureFilterToDkFilter(TextureFilter type) {
		switch(type) {
			case TEXTURE_FILTER_NEAREST: {
				return DkFilter_Nearest;
			}

			case TEXTURE_FILTER_LINEAR: {
				return DkFilter_Linear;
			}
		}
	}

	inline DkWrapMode textureWrapToDkWrap(TextureWrap wrap) {
		switch(wrap) {
			case TEXTURE_WRAP_REPEAT: {
				return DkWrapMode_Repeat;
			}

			case TEXTURE_WRAP_MIRRORED_REPEAT: {
				return DkWrapMode_MirroredRepeat;
			}

			case TEXTURE_WRAP_CLAMP_TO_EDGE: {
				return DkWrapMode_ClampToEdge;
			}

			case TEXTURE_WRAP_CLAMP_TO_BORDER: {
				return DkWrapMode_ClampToBorder;
			}

			case TEXTURE_WRAP_MIRROR_CLAMP_TO_EDGE: {
				return DkWrapMode_MirrorClampToEdge;
			}
		}
	}
	#else
	inline GLenum textureFilterToGLFilter(TextureFilter type) {
		switch(type) {
			case TEXTURE_FILTER_NEAREST: {
				return GL_NEAREST;
			}

			case TEXTURE_FILTER_LINEAR: {
				return GL_LINEAR;
			}
		}
	}

	inline GLenum textureWrapToGLWrap(TextureWrap wrap) {
		switch(wrap) {
			case TEXTURE_WRAP_REPEAT: {
				return GL_REPEAT;
			}

			case TEXTURE_WRAP_MIRRORED_REPEAT: {
				return GL_MIRRORED_REPEAT;
			}

			case TEXTURE_WRAP_CLAMP_TO_EDGE: {
				return GL_CLAMP_TO_EDGE;
			}

			case TEXTURE_WRAP_CLAMP_TO_BORDER: {
				return GL_CLAMP_TO_BORDER;
			}

			case TEXTURE_WRAP_MIRROR_CLAMP_TO_EDGE: {
				return GL_MIRROR_CLAMP_TO_EDGE;
			}
		}
	}
	#endif
	
	class Texture {
		friend class Window;
		
		public:
			Texture(class Window* window);

			void setFilters(TextureFilter minFilter, TextureFilter magFilter);
			void setWrap(TextureWrap uWrap, TextureWrap vWrap);

			void loadPNGFromFile(string filename);
			void loadPNG(const unsigned char* buffer, unsigned int size);
			void bind(unsigned int location);
		
		protected:
			Window* window;

			png_uint_32 width;
			png_uint_32 height;
			png_byte bytesPerPixel;
			png_byte colorType;
			png_byte bitDepth;
			png_byte* imageData;

			TextureFilter minFilter;
			TextureFilter magFilter;
			TextureWrap uWrap;
			TextureWrap vWrap;

			#ifdef __switch__
			dk::Image image;
			dk::ImageDescriptor imageDescriptor;
			dk::Sampler sampler;
			dk::SamplerDescriptor samplerDescriptor;
			switch_memory::Piece* memory;
			#else
			GLuint texture = GL_INVALID_INDEX;
			GLenum getFormat();
			GLenum getType();
			#endif

			void load(); // load into GL/deko3d structure
	};
};
