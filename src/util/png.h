#pragma once

#include <png.h>

struct png {
	unsigned char* buffer;
	unsigned int bufferSize;
	unsigned int width;
	unsigned int height;
	unsigned int bitDepth;
	unsigned int bytesPerPixel;
	unsigned int channels;
};

png loadPng(const unsigned char* buffer, unsigned int size);
