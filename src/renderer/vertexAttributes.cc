#ifndef __switch__
#include <glad/gl.h>
#endif

#include "vertexAttributes.h"
#include "vertexBuffer.h"

#include "window.h"

render::VertexAttributes::VertexAttributes(Window* window) {
	this->window = window;
}

void render::VertexAttributes::addVertexAttribute(class VertexBuffer* buffer, unsigned short attributeLocation, unsigned short vectorLength, VertexAttributeType type, unsigned short offset, unsigned short stride, unsigned short divisor) {
	this->attributes.push_back(VertexAttribute {
		buffer: buffer,
		attributeLocation: attributeLocation,
		vectorLength: vectorLength,
		type: type,
		offset: offset,
		stride: stride,
		divisor: divisor,
	});
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
			attributeTypeToDkAttribSize(attribute.type, attribute.vectorLength),
			attributeTypeToDkAttribType(attribute.type),
			0,
		});
	}
	#else
	if(this->window->backend == OPENGL_BACKEND) {
		if(this->vertexArrayObject != GL_INVALID_INDEX) {
			return;
		}

		glGenVertexArrays(1, &this->vertexArrayObject);
		glBindVertexArray(this->vertexArrayObject);
		
		for(VertexAttribute &attribute: this->attributes) {
			glBindBuffer(GL_ARRAY_BUFFER, attribute.buffer->bufferId);
			if(attribute.type == VERTEX_ATTRIB_HALF_FLOAT || attribute.type == VERTEX_ATTRIB_FLOAT || attribute.type == VERTEX_ATTRIB_DOUBLE) {
				glVertexAttribPointer(attribute.attributeLocation, attribute.vectorLength, attributeTypeToGLType(attribute.type), GL_FALSE, attribute.stride, 0);
			}
			else {
				glVertexAttribIPointer(attribute.attributeLocation, attribute.vectorLength, attributeTypeToGLType(attribute.type), attribute.stride, 0);
			}

			if(attribute.divisor) {
				glVertexAttribDivisor(attribute.attributeLocation, attribute.divisor);
			}

			glEnableVertexAttribArray(attribute.attributeLocation);
		}

		glBindVertexArray(0);
	}
	else {
		if(this->inputBindings.size() == 0) {
			unsigned short bufferId = 0;
			VertexBuffer* currentBuffer = nullptr;
			bool incrementBufferId = this->attributes[0].buffer;

			for(VertexAttribute &attribute: this->attributes) {
				if(currentBuffer != attribute.buffer) {
					this->inputBindings.push_back(vk::VertexInputBindingDescription(
						bufferId,
						attribute.stride,
						attribute.divisor == 0 ? vk::VertexInputRate::eVertex : vk::VertexInputRate::eInstance
					));
					incrementBufferId = true;
				}

				this->inputAttributes.push_back(vk::VertexInputAttributeDescription(
					attribute.attributeLocation,
					bufferId,
					attributeTypeToVulkanType(attribute.type, attribute.vectorLength),
					attribute.offset
				));

				if(incrementBufferId) {
					bufferId++;
				}
			}
		}
	}
	#endif
}
