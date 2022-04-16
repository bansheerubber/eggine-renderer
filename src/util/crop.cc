#include "crop.h"

cropped crop(png &image, unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
	png_byte* output = new png_byte[width * height * image.bytesPerPixel];
	for(unsigned int j = y; j < y + height; j++) {
		for(unsigned int i = x * image.bytesPerPixel; i < (x + width) * image.bytesPerPixel; i++) {
			unsigned int outputIndex = (i - x * image.bytesPerPixel) % (width * image.bytesPerPixel) + (j - y) * width * image.bytesPerPixel;
			unsigned int inputIndex = i % (image.width * image.bytesPerPixel) + j * image.width * image.bytesPerPixel;

			if((y / 6) % 2 == 0) {
				output[outputIndex] = 255;
			}

			output[outputIndex] = image.buffer[inputIndex];
		}
	}
	return {
		buffer: output,
		bufferSize: width * height * image.bytesPerPixel,
		width: width,
		height: height,
		source: image,
	};
}
