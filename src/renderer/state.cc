#ifndef __switch__
#include <glad/gl.h>
#endif

#include "state.h"

#include "../engine/console.h"
#include "program.h"
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
		
		render::VulkanPipeline pipeline = { this->window, this->current.primitive, (float)this->window->width, (float)this->window->height, this->current.program };
		if(this->window->pipelineCache.find(pipeline) == this->window->pipelineCache.end()) {
			this->window->pipelineCache[pipeline] = pipeline.newPipeline(); // TODO move this creation step to the window class??
		}

		this->buffer[this->window->framePingPong].bindPipeline(vk::PipelineBindPoint::eGraphics, *this->window->pipelineCache[pipeline].pipeline);

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
