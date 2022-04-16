#include "png.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../engine/console.h"

struct PNGBuffer {
	const unsigned char* buffer;
	size_t currentIndex;
};

void copyToPNGBuffer(png_structp png, png_bytep output, png_size_t size) {
	PNGBuffer* buffer = (PNGBuffer*)png_get_io_ptr(png);
	if(buffer == NULL) {
		console::error("could not load PNG io pointer\n");
		return;
	}

	memcpy(output, &buffer->buffer[buffer->currentIndex], size);
	buffer->currentIndex += size;
}

png loadPng(const unsigned char* buffer, unsigned int size) {
	if(png_sig_cmp((unsigned char*)buffer, 0, 8)) {
		console::error("could not recognize as PNG\n");
		return {
			buffer: nullptr,
		};
	}

	/* initialize stuff */
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if(!png) {
		console::error("could not initialize PNG reader\n");
		return {
			buffer: nullptr,
		};
	}

	png_infop info = png_create_info_struct(png);
	if(!info) {
		console::error("could not load PNG info\n");
		return {
			buffer: nullptr,
		};
	}

	if(setjmp(png_jmpbuf(png))) {
		console::error("error reading PNG info");
		return {
			buffer: nullptr,
		};
	}

	PNGBuffer* pngBuffer = new PNGBuffer;
	pngBuffer->buffer = buffer;
	pngBuffer->currentIndex = 8;

	png_set_read_fn(png, (void*)pngBuffer, &copyToPNGBuffer);
	png_set_sig_bytes(png, 8);

	png_read_info(png, info);

	png_byte colorType = png_get_color_type(png, info);
	png_byte bitDepth = png_get_bit_depth(png, info);

	if(bitDepth < 8) {
		png_set_packing(png);
	}

	if(colorType == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png);
		png_set_add_alpha(png, 255, PNG_FILLER_AFTER);
	}

	if(png_get_valid(png, info, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(png);
	}

	png_read_update_info(png, info);

	png_byte channels = png_get_channels(png, info);
	png_uint_32 width = png_get_image_width(png, info);
	png_uint_32 height = png_get_image_height(png, info);

	/* read file */
	if(setjmp(png_jmpbuf(png))) {
		console::error("could not read PNG image\n");
		return {
			buffer: nullptr,
		};
	}

	png_byte** image = (png_bytep*)malloc(sizeof(png_bytep) * height);
	for(png_uint_32 y = 0; y < height; y++) {
		image[y] = (png_byte*)malloc(png_get_rowbytes(png, info));
	}

	png_read_image(png, image);

	png_byte* imageData = new png_byte[width * channels * height];
	for(png_uint_32 y = 0; y < height; y++) {
		for(png_uint_32 x = 0; x < width * channels; x++) {
			imageData[y * width * channels + x % (width * channels)] = image[y][x];
		}
	}

	for(png_uint_32 y = 0; y < height; y++) {
		free(image[y]);
	}
	free(image);

	png_destroy_read_struct(&png, &info, NULL);

	return {
		buffer: imageData,
		bufferSize: width * channels * height,
		width: width,
		height: height,
		bitDepth: bitDepth,
		bytesPerPixel: channels,
		channels: channels,
	};
}
