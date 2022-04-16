#pragma once

#include <png.h>

#include "png.h"

struct cropped {
	png_byte* buffer;
	unsigned int bufferSize;
	unsigned int width;
	unsigned int height;
	png &source;
};

cropped crop(png &image, unsigned int x, unsigned int y, unsigned int width, unsigned int height);
