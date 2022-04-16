#ifdef __switch__
#include <string.h>
#else
#include <glad/gl.h>
#endif

#include "../util/align.h"
#include "vertexBuffer.h"
#include "window.h"

render::VertexBuffer::VertexBuffer(Window* window) {
	this->window = window;
}

render::VertexBuffer::~VertexBuffer() {
	this->destroyBuffer();
}

void render::VertexBuffer::setDynamicDraw(bool isDynamicDraw) {
	#ifndef __switch__
	this->usage = isDynamicDraw ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
	#endif
}

void render::VertexBuffer::setData(void* data, unsigned int size, unsigned int align) {
	this->size = size;
	
	#ifdef __switch__
	this->align = align;
	
	if(!this->memoryAllocated || alignTo(size, align) != this->memory->size()) {
		if(this->memory != nullptr) {
			this->memory->deallocate(); // deallocate memory
		}
		
		this->memory = this->window->memory.allocate(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, this->size, align);
	}

	if(data != nullptr) {
		memcpy(this->memory->cpuAddr(), data, this->size);
	}
	this->memoryAllocated = true;
	#else
	if(this->bufferId == GL_INVALID_INDEX) {
		if(this->usage == GL_DYNAMIC_DRAW) {
			this->createDynamicBuffer();
		}
		else {
			this->createBuffer();
		}
	}

	if(this->usage == GL_DYNAMIC_DRAW) {
		glBindBuffer(GL_ARRAY_BUFFER, this->bufferId);
		glBufferData(GL_ARRAY_BUFFER, this->size, NULL, this->usage); // orphan the buffer
		glBufferSubData(GL_ARRAY_BUFFER, 0, this->size, data);

		#ifdef RENDER_UNBIND_VERTEX_BUFFERS
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		#endif
	}
	else {
		glBindBuffer(GL_ARRAY_BUFFER, this->bufferId);
		glBufferData(GL_ARRAY_BUFFER, this->size, data, this->usage);

		#ifdef RENDER_UNBIND_VERTEX_BUFFERS
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		#endif
	}
	#endif
}

#ifdef __switch__
void render::VertexBuffer::bind(unsigned short id) {
	this->window->commandBuffer.bindVtxBuffer(id, this->memory->gpuAddr(), this->memory->size());
}
#else
void render::VertexBuffer::bind() {
	glBindBuffer(GL_ARRAY_BUFFER, this->bufferId);
}
#endif

void render::VertexBuffer::createBuffer() {
	#ifndef __switch__
	this->destroyBuffer();
	
	this->usage = GL_STATIC_DRAW;
	glGenBuffers(1, &this->bufferId);
	#endif
}

void render::VertexBuffer::createDynamicBuffer() {
	#ifndef __switch__
	this->destroyBuffer();
	
	this->usage = GL_DYNAMIC_DRAW;
	glGenBuffers(1, &this->bufferId);
	glBindBuffer(GL_ARRAY_BUFFER, this->bufferId);
	glBufferData(GL_ARRAY_BUFFER, this->size, NULL, this->usage);

	#ifdef RENDER_UNBIND_VERTEX_BUFFERS
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	#endif
	#endif
}

void render::VertexBuffer::destroyBuffer() {
	#ifdef __switch__
	if(this->memory != nullptr) {
		this->memory->deallocate();
	}
	this->memoryAllocated = false;
	#else
	if(this->bufferId != GL_INVALID_INDEX) {
		glDeleteBuffers(1, &this->bufferId);
	}
	#endif
}
