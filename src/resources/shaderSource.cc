#include "shaderSource.h"

#include <string.h>

resources::ShaderSource::ShaderSource(const unsigned char* buffer, uint64_t bufferSize) {
	this->buffer = new unsigned char[bufferSize];
	this->bufferSize = bufferSize;
	memcpy(this->buffer, buffer, bufferSize);
}
