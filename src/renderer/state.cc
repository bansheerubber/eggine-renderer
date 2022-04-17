#ifndef __switch__
#include <glad/gl.h>
#endif

#include "state.h"

#include "../engine/console.h"
#include "program.h"
#include "vertexAttributes.h"
#include "vertexBuffer.h"
#include "window.h"

render::State::State() {

}

render::State::State(render::Window* window) {
	this->window = window;
}

void render::State::reset() {
	this->applied = false;

	if(this->window->backend != VULKAN_BACKEND) {
		return;
	}

	this->buffer[this->window->framePingPong].reset();
}

void render::State::bindPipeline() {
	if(this->window->backend != VULKAN_BACKEND || this->window->swapchainOutOfDate) {
		return;
	}

	if(this->current != this->old || !this->applied) {
		if(this->current.program == nullptr) {
			console::print("render state: no program bound\n");
			exit(1);
		}
		
		render::VulkanPipeline pipeline = {
			this->window,
			this->current.primitive,
			(float)this->window->width,
			(float)this->window->height,
			this->current.program,
			this->current.attributes,
		};
		if(this->window->pipelineCache.find(pipeline) == this->window->pipelineCache.end()) {
			this->window->pipelineCache[pipeline] = pipeline.newPipeline(); // TODO move this creation step to the window class??
		}

		this->buffer[this->window->framePingPong].bindPipeline(vk::PipelineBindPoint::eGraphics, *this->window->pipelineCache[pipeline].pipeline);

		// probably should move vertex buffer binding away from here
		std::vector<vk::Buffer> vertexBuffers;
		std::vector<vk::DeviceSize> offsets;
		for(VertexAttribute attribute: this->current.attributes->attributes) {
			vertexBuffers.push_back(attribute.buffer->buffer);
			offsets.push_back(0);
		}

		this->buffer[this->window->framePingPong].bindVertexBuffers(0, vertexBuffers.size(), vertexBuffers.data(), offsets.data());

		this->old = this->current;
	}
}

void render::State::draw(PrimitiveType type, unsigned int firstVertex, unsigned int vertexCount, unsigned int firstInstance, unsigned int instanceCount) {
	this->current.primitive = type;

	this->bindPipeline();
	
	if(this->window->backend == OPENGL_BACKEND) {
		glDrawArraysInstancedBaseInstance(primitiveToGLPrimitive(type), firstVertex, vertexCount, instanceCount, firstInstance);
	}
	else if(!this->window->swapchainOutOfDate) {
		this->buffer[this->window->framePingPong].draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}
}

void render::State::bindProgram(render::Program* program) {
	this->current.program = program;
	
	if(this->window->backend == OPENGL_BACKEND) {
		program->compile();
		glUseProgram(this->current.program->program);
	}
}

void render::State::bindVertexAttributes(render::VertexAttributes* attributes) {
	this->current.attributes = attributes;

	attributes->buildCommandLists();

	#ifdef __switch__
	unsigned short id = 0;
	for(VertexBuffer* buffer: this->bufferBindOrder) {
		buffer->bind(id++);
	}

	this->window->commandBuffer.bindVtxAttribState(dk::detail::ArrayProxy(this->attributeStates.size(), (const DkVtxAttribState*)this->attributeStates.data()));
	this->window->commandBuffer.bindVtxBufferState(dk::detail::ArrayProxy(this->bufferStates.size(), (const DkVtxBufferState*)this->bufferStates.data()));
	#else
	if(this->window->backend == OPENGL_BACKEND) {
		glBindVertexArray(attributes->vertexArrayObject);
	}
	#endif
}
