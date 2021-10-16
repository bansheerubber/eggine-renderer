#include "vertexAttributes.h"
#include "vertexBuffer.h"

#include "window.h"

render::VertexAttributes::VertexAttributes(Window* window) {
	this->window = window;
}

void render::VertexAttributes::addVertexAttribute(class VertexBuffer* buffer, unsigned short attributeLocation, unsigned short vectorLength, unsigned short dataSize, unsigned short offset, unsigned short stride, unsigned short divisor) {
	this->attributes.push_back(VertexAttribute {
		buffer: buffer,
		attributeLocation: attributeLocation,
		vectorLength: vectorLength,
		dataSize: dataSize,
		offset: offset,
		stride: stride,
		divisor: divisor,
	});
}

void render::VertexAttributes::bind() {
	#ifdef __switch__
	this->buildCommandLists();

	unsigned short id = 0;
	for(VertexBuffer* buffer: this->bufferBindOrder) {
		buffer->bind(id++);
	}

	this->window->commandBuffer.bindVtxAttribState(dk::detail::ArrayProxy(this->attributeStates.size(), (const DkVtxAttribState*)this->attributeStates.data()));
	this->window->commandBuffer.bindVtxBufferState(dk::detail::ArrayProxy(this->bufferStates.size(), (const DkVtxBufferState*)this->bufferStates.data()));
	#endif
}

void render::VertexAttributes::buildCommandLists() {
	#ifdef __switch__
	this->bufferBindOrder.clear();
	this->attributeStates.clear();
	this->bufferStates.clear();
	
	unsigned short bufferId = 0;
	unsigned short nextBufferId = 0;
	VertexBuffer* currentBuffer = nullptr;
	
	// iterate through the attributes and build the command lists
	for(VertexAttribute &attribute: this->attributes) {
		if(currentBuffer != attribute.buffer) {
			currentBuffer = attribute.buffer;
			this->bufferBindOrder.push_back(currentBuffer);
			this->bufferStates.push_back(DkVtxBufferState {
				attribute.stride,
				attribute.divisor,
			});
			bufferId = nextBufferId;
			nextBufferId++;
		}

		this->attributeStates.push_back(DkVtxAttribState {
			bufferId,
			0,
			attribute.offset,
			DkVtxAttribSize_3x32,
			DkVtxAttribType_Float,
			0,
		});
	}
	#endif
}
