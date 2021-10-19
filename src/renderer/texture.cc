#ifndef __switch__
#include <glad/gl.h>
#endif

#include "texture.h"

#include <fstream>
#include <stdlib.h>
#include <string.h>

#include "window.h"

render::Texture::Texture(Window* window) {
	this->window = window;
}

void render::Texture::setFilters(TextureFilter minFilter, TextureFilter magFilter) {
	this->minFilter = minFilter;
	this->magFilter = magFilter;
}

void render::Texture::setWrap(TextureWrap uWrap, TextureWrap vWrap) {
	this->uWrap = uWrap;
	this->vWrap = vWrap;
}

void render::Texture::loadPNGFromFile(string filename) {
	ifstream file(filename);

	if(file.bad() || file.fail()) {
		printf("failed to open file for png %s\n", filename.c_str());
		file.close();
		this->window->addError();
		return;
	}

	file.seekg(0, file.end);
	unsigned long length = file.tellg();
	file.seekg(0, file.beg);
	char* buffer = new char[length];
	file.read((char*)buffer, length);
	file.close();

	this->loadPNG((unsigned char*)buffer, length);

	delete[] buffer;	
}

struct PNGBuffer {
	const unsigned char* buffer;
	size_t currentIndex;
};

void copyDataToPNGBuffer(png_structp png, png_bytep output, png_size_t size) {
	PNGBuffer* buffer = (PNGBuffer*)png_get_io_ptr(png);
	if(buffer == NULL) {
		printf("could not load PNG io pointer\n");
		return;
	}

	memcpy(output, &buffer->buffer[buffer->currentIndex], size);
	buffer->currentIndex += size;
}

void render::Texture::Texture::loadPNG(const unsigned char* buffer, unsigned int size) {
	if(png_sig_cmp(buffer, 0, 8)) {
		printf("could not recognize as PNG\n");
		return;
	}

	/* initialize stuff */
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if(!png) {
		printf("could not initialize PNG reader\n");
		return;
	}

	png_infop info = png_create_info_struct(png);
	if(!info) {
		printf("could not load PNG info\n");
		return;
	}

	if(setjmp(png_jmpbuf(png))) {
		printf("error reading PNG info");
		return;
	}

	PNGBuffer* pngBuffer = new PNGBuffer;
	pngBuffer->buffer = buffer;
	pngBuffer->currentIndex = 8;

	png_set_read_fn(png, (void*)pngBuffer, &copyDataToPNGBuffer);
	png_set_sig_bytes(png, 8);

	png_read_info(png, info);

	this->width = png_get_image_width(png, info);
	this->height = png_get_image_height(png, info);
	this->colorType = png_get_color_type(png, info);
	this->bitDepth = png_get_bit_depth(png, info);

	int bytesPerPixel = 0;
	if(this->colorType == PNG_COLOR_TYPE_RGB) {
		bytesPerPixel = 3;
	}
	else if(this->colorType == PNG_COLOR_TYPE_RGB_ALPHA) {
		bytesPerPixel = 4;
	}
	else {
		printf("PNG format not supported\n");
		return;
	}

	png_read_update_info(png, info);

	/* read file */
	if(setjmp(png_jmpbuf(png))) {
		printf("could not read PNG image\n");
		return;
	}

	png_byte** image = (png_bytep*)malloc(sizeof(png_bytep) * height);
	for(png_uint_32 y = 0; y < this->height; y++) {
		image[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
	}

	png_read_image(png, image);

	this->bytesPerPixel = bytesPerPixel;
	this->imageData = new png_byte[this->width * bytesPerPixel * this->height];
	png_uint_32 index = 0;
	for(png_uint_32 y = 0; y < this->height; y++) {
		for(png_uint_32 x = 0; x < this->width * bytesPerPixel; x++) {
			this->imageData[y * this->width * bytesPerPixel + x % (this->width * bytesPerPixel)] = image[y][x];
		}
	}

	for(png_uint_32 y = 0; y < this->height; y++) {
		free(image[y]);
	}
	free(image);

	png_destroy_read_struct(&png, &info, NULL);

	this->load();
}

void render::Texture::load() {
	if(this->minFilter == TEXTURE_FILTER_INVALID || this->magFilter == TEXTURE_FILTER_INVALID) {
		printf("invalid texture filters\n");
		return;
	}

	if(this->uWrap == TEXTURE_WRAP_INVALID || this->vWrap == TEXTURE_WRAP_INVALID) {
		printf("invalid texture wraps\n");
		return;
	}

	#ifdef __switch__
	// allocate memory for the image, we will be deallocating this later though
	switch_memory::Piece* memory = this->window->memory.allocate(
		DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached,
		this->width * this->height * this->bytesPerPixel,
		DK_IMAGE_LINEAR_STRIDE_ALIGNMENT
	);
	memcpy(memory->cpuAddr(), this->imageData, this->width * this->height * this->bytesPerPixel);

	dk::ImageLayout layout;
	dk::ImageLayoutMaker{this->window->device}
		.setFlags(0)
		.setFormat(DkImageFormat_RGBA8_Uint)
		.setDimensions(this->width, this->height)
		.initialize(layout);

	// // allocate the actual image that we won't get rid of
	this->memory = this->window->memory.allocate(DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image, layout.getSize(), layout.getAlignment());
	this->image.initialize(layout, this->memory->parent->block, this->memory->start);
	this->imageDescriptor.initialize(this->image);

	this->sampler.setFilter(textureFilterToDkFilter(this->minFilter), textureFilterToDkFilter(this->magFilter));
	this->sampler.setWrapMode(textureWrapToDkWrap(this->uWrap), textureWrapToDkWrap(this->vWrap));
	this->samplerDescriptor.initialize(this->sampler);

	dk::ImageView view{this->image};
	this->window->addTexture(memory, view, this->width, this->height);

	memory->deallocate();
	#else
	// load the GL texture
	glGenTextures(1, &this->texture);
	glBindTexture(GL_TEXTURE_2D, this->texture);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		this->getFormat(),
		this->width,
		this->height,
		0,
		this->getFormat(),
		this->getType(),
		this->imageData
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureWrapToGLWrap(this->uWrap));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureWrapToGLWrap(this->vWrap));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureFilterToGLFilter(this->minFilter));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureFilterToGLFilter(this->magFilter));
	#endif

	delete[] this->imageData;
}

void render::Texture::bind(unsigned int location) {
	#ifdef __switch__
	this->window->bindTexture(location, this);
	#else
	glActiveTexture(GL_TEXTURE0 + location);
	glBindTexture(GL_TEXTURE_2D, this->texture);
	#endif
}

#ifndef __switch__
GLenum render::Texture::getFormat() {
	if(this->colorType == PNG_COLOR_TYPE_RGB) {
		return GL_RGB;
	}
	else if(this->colorType == PNG_COLOR_TYPE_RGB_ALPHA) {
		return GL_RGBA;
	}
	else {
		return GL_INVALID_INDEX;
	}
}

GLenum render::Texture::getType() {
	if(this->bitDepth == 8) {
		return GL_UNSIGNED_BYTE;
	}
	else if(this->bitDepth == 16) {
		return GL_UNSIGNED_SHORT;
	}
	else {
		return GL_INVALID_INDEX;
	}
}
#endif
