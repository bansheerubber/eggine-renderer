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
	this->isDynamicDraw = isDynamicDraw;
	this->oldSize = (uint32_t)-1; // force buffer reallocation
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
	if(this->window->backend == OPENGL_BACKEND) {
		if(this->bufferId == GL_INVALID_INDEX) {
			this->destroyBuffer();
			if(this->usage == GL_DYNAMIC_DRAW) {
				this->usage = GL_DYNAMIC_DRAW;
				glGenBuffers(1, &this->bufferId);
				glBindBuffer(GL_ARRAY_BUFFER, this->bufferId);
				glBufferData(GL_ARRAY_BUFFER, this->size, NULL, this->usage);

				#ifdef RENDER_UNBIND_VERTEX_BUFFERS
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				#endif
			}
			else {
				this->usage = GL_STATIC_DRAW;
				glGenBuffers(1, &this->bufferId);
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
	}
	else {
		if(this->size != this->oldSize) {
			this->destroyBuffer();
			
			if(this->isDynamicDraw) { // do not use staging buffer if we're dynamic draw
				this->gpuBuffer = this->window->memory.allocate(
					vk::BufferCreateInfo(
						{},
						size,
						vk::BufferUsageFlagBits::eVertexBuffer,
						vk::SharingMode::eExclusive
					),
					vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
				);
			}
			else {
				this->stagingBuffer = this->window->memory.allocate(
					vk::BufferCreateInfo(
						{},
						size,
						vk::BufferUsageFlagBits::eTransferSrc,
						vk::SharingMode::eExclusive
					),
					vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
				);
				
				this->gpuBuffer = this->window->memory.allocate(
					vk::BufferCreateInfo(
						{},
						size,
						vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
						vk::SharingMode::eExclusive
					),
					vk::MemoryPropertyFlagBits::eDeviceLocal
				);
			}
		}

		if(this->isDynamicDraw) {
			memcpy(this->gpuBuffer->map(), data, this->size);
		}
		else {
			memcpy(this->stagingBuffer->map(), data, this->size);
			this->needsCopy = true;
		}
	}
	#endif

	this->oldSize = this->size;
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

void render::VertexBuffer::destroyBuffer() {
	#ifdef __switch__
	if(this->memory != nullptr) {
		this->memory->deallocate();
	}
	this->memoryAllocated = false;
	#else
	if(this->window->backend == OPENGL_BACKEND) {
		if(this->bufferId != GL_INVALID_INDEX) {
			glDeleteBuffers(1, &this->bufferId);
		}
	}
	else {
		if(this->oldSize != 0) {
			if(this->stagingBuffer != nullptr) {
				this->stagingBuffer->deallocate();
			}
			
			if(this->gpuBuffer != nullptr) {
				this->gpuBuffer->deallocate();
			}
		}
	}
	#endif
}
