#ifndef __switch__
#include <glad/gl.h>
#endif

#include "program.h"

#include "../engine/console.h"
#include "shader.h"
#include "window.h"

#ifndef __switch__
unsigned int render::Program::UniformCount = 0;
#endif

render::Program::Program(Window* window) {
	this->window = window;
	this->window->programs.push_back(this);
}

void render::Program::addShader(Shader* shader) {
	this->shaders.push_back(shader);

	#ifndef __switch__
	if(this->window->backend == VULKAN_BACKEND) {
		this->stages[shader->type == SHADER_VERTEX ? 0 : 1] = vk::PipelineShaderStageCreateInfo(
			{},
			shader->type == SHADER_VERTEX ? vk::ShaderStageFlagBits::eVertex : vk::ShaderStageFlagBits::eFragment,
			shader->module,
			"main"
		);
	}
	#endif
}

void render::Program::compile() {
	#ifndef __switch__
	if(this->window->backend == OPENGL_BACKEND) {
		if(this->program == GL_INVALID_INDEX) {
			GLuint program = glCreateProgram();
			
			for(Shader* shader: this->shaders) {
				if(shader->shader == GL_INVALID_INDEX) {
					console::error("shaders not compiled\n");
					exit(1);
				}

				glAttachShader(program, shader->shader);
			}

			glLinkProgram(program);

			GLint linked = 0;
			glGetProgramiv(program, GL_LINK_STATUS, &linked);
			if(linked == GL_FALSE) {
				// print the error log
				GLint logLength = 0;
				glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

				GLchar* log = new GLchar[logLength];
				glGetProgramInfoLog(program, logLength, &logLength, log);

				glDeleteProgram(program);
				console::error("failed to link program:\n%.*s\n", logLength, log);

				this->program = GL_INVALID_INDEX - 1;

				exit(1);
			}
			else {
				this->program = program;
				this->compiled = true;

				for(Shader* shader: this->shaders) {
					glDetachShader(program, shader->shader);

					// handle uniform block bindings (reconcile differences between deko3d and opengl)
					for(auto &[uniform, binding]: shader->uniformToBinding) {
						// update shader binding for opengl
						GLuint blockIndex = glGetUniformBlockIndex(this->program, uniform.c_str());
						if(blockIndex != GL_INVALID_INDEX) {
							glUniformBlockBinding(this->program, blockIndex, binding + UniformCount);
							this->uniformToBinding[uniform] = binding + UniformCount;
						}
					}
					UniformCount += shader->uniformToBinding.size();
				}
			}
		}
	}
	else if(!this->compiled) {
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		for(Shader* shader: this->shaders) {
			for(auto &[uniform, binding]: shader->uniformToBinding) {
				bindings.push_back(vk::DescriptorSetLayoutBinding(
					binding,
					vk::DescriptorType::eUniformBuffer,
					1,
					vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
					nullptr
				));

				this->uniformToShaderBinding[uniform] = binding;
			}
		}

		if(bindings.size() != 0) {
			vk::DescriptorSetLayoutCreateInfo createInfo({}, bindings.size(), bindings.data());
			this->descriptorLayout = this->window->device.device.createDescriptorSetLayout(createInfo);
		}

		this->compiled = true;
	}
	#endif
}

void render::Program::bindTexture(std::string uniformName, unsigned int texture) {
	#ifdef __switch__
	this->window->commandBuffer.bindTextures(DkStage_Fragment, 0, dkMakeTextureHandle(0, 0));
	#else
	if(this->window->backend == OPENGL_BACKEND) {
		// get the location of the uniform we're going to bind to
		GLuint location = glGetUniformLocation(this->program, uniformName.c_str());
		glUniform1i(location, texture);
	}
	#endif
}

#ifndef __switch__
void render::Program::createDescriptorSet() {
	if(this->descriptorSetInitialized || this->uniformToShaderBinding.size() == 0) {
		return;
	}

	vk::DescriptorSetAllocateInfo allocateInfo(this->window->descriptorPool, 1, &this->descriptorLayout);
	this->descriptorSet = this->window->device.device.allocateDescriptorSets(allocateInfo)[0];

	std::vector<vk::WriteDescriptorSet> writes;
	for(auto &[uniform, binding]: this->uniformToShaderBinding) {
		vk::DescriptorBufferInfo bufferInfo(this->uniformToVulkanBuffer[uniform].getBuffer(), 0, VK_WHOLE_SIZE);
		vk::WriteDescriptorSet descriptorWrite(
			this->descriptorSet, binding, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bufferInfo, nullptr
		);
		writes.push_back(descriptorWrite);
	}
	this->window->device.device.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);

	this->descriptorSetInitialized = true;
}
#endif

void render::Program::createUniformBuffer(std::string uniformName, unsigned int size) {
	#ifdef __switch__
	this->uniformToPiece[uniformName] = this->window->memory.allocate(
		DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, size, DK_UNIFORM_BUF_ALIGNMENT
	);
	#else
	if(this->window->backend == OPENGL_BACKEND) {
		GLuint bufferId;
		glGenBuffers(1, &bufferId);
		this->uniformToBuffer[uniformName] = bufferId;

		glBindBuffer(GL_UNIFORM_BUFFER, this->uniformToBuffer[uniformName]);
		glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, this->uniformToBinding.find(uniformName).value(), bufferId);
	}
	else if(this->window->backend == VULKAN_BACKEND) {
		this->uniformToVulkanBuffer[uniformName] = this->window->allocateVulkanBuffer(
			vk::BufferCreateInfo(
				{},
				size,
				vk::BufferUsageFlagBits::eUniformBuffer,
				vk::SharingMode::eExclusive
			),
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);
	}
	#endif
}
