#ifndef __switch__
#include <glad/gl.h>
#endif

#include "state.h"

#include "../engine/console.h"
#include "program.h"
#include "shader.h"
#include "vertexAttributes.h"
#include "vertexBuffer.h"
#include "window.h"

render::State::State() {

}

render::State::State(render::Window* window) {
	this->window = window;
}

void render::State::draw(PrimitiveType type, unsigned int firstVertex, unsigned int vertexCount, unsigned int firstInstance, unsigned int instanceCount) {
	this->current.primitive = type;

	#ifdef __switch__
	this->window->commandBuffer.draw(primitiveToDkPrimitive(type), vertexCount, instanceCount, firstVertex, firstInstance);
	#else
	this->bindPipeline();
	
	if(this->window->backend == OPENGL_BACKEND) {
		glDrawArraysInstancedBaseInstance(primitiveToGLPrimitive(type), firstVertex, vertexCount, instanceCount, firstInstance);
	}
	else if(!this->window->swapchainOutOfDate) {
		this->buffer[this->window->framePingPong].draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}
	#endif
}

void render::State::bindProgram(render::Program* program) {
	this->current.program = program;
	
	#ifdef __switch__
	std::vector<DkShader const*> shaders;
	for(Shader* shader: program->shaders) {
		shaders.push_back(&shader->shader);
	}
	
	dkCmdBufBindShaders(this->window->commandBuffer, DkStageFlag_GraphicsMask, shaders.data(), shaders.size());

	for(Shader* shader: program->shaders) {
		for(auto &[uniform, binding]: shader->uniformToBinding) {
			program->uniformToBinding[uniform] = binding;
		}
	}
	#else
	program->compile();
	if(this->window->backend == OPENGL_BACKEND) {
		glUseProgram(this->current.program->program);
	}
	#endif
}

void render::State::bindUniform(std::string uniformName, void* data, uint32_t size) {
	#ifdef __switch__
	if(this->current.program->uniformToPiece.find(uniformName) == this->current.program->uniformToPiece.end()) {
		this->current.program->createUniformBuffer(uniformName, size);
	}

	this->window->commandBuffer.pushConstants(
		this->current.program->uniformToPiece[uniformName]->gpuAddr(),
		this->current.program->uniformToPiece[uniformName]->size(),
		0,
		size,
		data
	);

	// look for binding
	for(Shader* shader: this->current.program->shaders) {
		if(shader->uniformToBinding.find(uniformName) != shader->uniformToBinding.end()) {
			this->window->commandBuffer.bindUniformBuffer(
				shader->type == SHADER_FRAGMENT ? DkStage_Fragment : DkStage_Vertex,
				shader->uniformToBinding[uniformName],
				this->current.program->uniformToPiece[uniformName]->gpuAddr(),
				this->current.program->uniformToPiece[uniformName]->size()
			);
		}
	}
	#else
	if(this->window->backend == OPENGL_BACKEND) {
		auto found = this->current.program->uniformToBuffer.find(uniformName);
		if(found == this->current.program->uniformToBuffer.end()) {
			this->current.program->createUniformBuffer(uniformName, size);
			found = this->current.program->uniformToBuffer.find(uniformName);
		}

		glBindBuffer(GL_UNIFORM_BUFFER, found.value());
		// glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW); // orphan the buffer
		glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
	}
	else if(this->window->backend == VULKAN_BACKEND) {
		auto found = this->current.program->uniformToVulkanBuffer.find(uniformName);
		if(found == this->current.program->uniformToVulkanBuffer.end()) {
			this->current.program->createUniformBuffer(uniformName, size);
			found = this->current.program->uniformToVulkanBuffer.find(uniformName);
		}
		
		memcpy(found.value().map(), data, size);
		found.value().unmap();
	}
	#endif
}

void render::State::bindVertexAttributes(render::VertexAttributes* attributes) {
	this->current.attributes = attributes;

	attributes->buildCommandLists();

	#ifdef __switch__
	unsigned short id = 0;
	for(VertexBuffer* buffer: attributes->bufferBindOrder) {
		buffer->bind(id++);
	}

	this->window->commandBuffer.bindVtxAttribState(
		dk::detail::ArrayProxy(attributes->attributeStates.size(), (const DkVtxAttribState*)attributes->attributeStates.data())
	);
	this->window->commandBuffer.bindVtxBufferState(
		dk::detail::ArrayProxy(attributes->bufferStates.size(), (const DkVtxBufferState*)attributes->bufferStates.data())
	);
	#else
	if(this->window->backend == OPENGL_BACKEND) {
		glBindVertexArray(attributes->vertexArrayObject);
	}
	#endif
}

void render::State::reset() {
	this->applied = false;

	#ifdef __switch__
	#else
	if(this->window->backend != VULKAN_BACKEND) {
		return;
	}

	this->buffer[this->window->framePingPong].reset();
	#endif
}

void render::State::bindPipeline() {
	#ifndef __switch__
	if(this->window->backend != VULKAN_BACKEND || this->window->swapchainOutOfDate) {
		return;
	}

	this->current.program->createDescriptorSet();

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

		this->buffer[this->window->framePingPong].bindPipeline(
			vk::PipelineBindPoint::eGraphics, *this->window->pipelineCache[pipeline].pipeline
		);

		if(this->current.program->descriptorSetInitialized) {
			this->buffer[this->window->framePingPong].bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics, *this->window->pipelineCache[pipeline].layout, 0, 1, &this->current.program->descriptorSet, 0, nullptr
			);
		}

		// probably should move vertex buffer binding away from here
		std::vector<vk::Buffer> vertexBuffers;
		std::vector<vk::DeviceSize> offsets;
		for(VertexAttribute attribute: this->current.attributes->attributes) {
			// figure out if we need to copy buffer contents
			if(attribute.buffer->needsCopy) {
				this->window->copyVulkanBuffer(attribute.buffer->stagingBuffer, attribute.buffer->gpuBuffer);
				attribute.buffer->needsCopy = false;
			}
			
			vertexBuffers.push_back(attribute.buffer->gpuBuffer.getBuffer());
			offsets.push_back(0);
		}

		this->buffer[this->window->framePingPong].bindVertexBuffers(0, vertexBuffers.size(), vertexBuffers.data(), offsets.data());

		this->old = this->current;
	}
	#endif
}
