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
}

void render::Program::addShader(Shader* shader) {
	this->shaders.push_back(shader);

	if(this->window->backend == VULKAN_BACKEND) {
		this->stages[shader->type == SHADER_VERTEX ? 0 : 1] = vk::PipelineShaderStageCreateInfo(
			{},
			shader->type == SHADER_VERTEX ? vk::ShaderStageFlagBits::eVertex : vk::ShaderStageFlagBits::eFragment,
			shader->module,
			"main"
		);
	}
}

void render::Program::bind() {
	#ifdef __switch__
	vector<DkShader const*> shaders;
	for(Shader* shader: this->shaders) {
		shaders.push_back(&shader->shader);
	}
	
	dkCmdBufBindShaders(this->window->commandBuffer, DkStageFlag_GraphicsMask, shaders.data(), shaders.size());

	for(Shader* shader: this->shaders) {
		for(auto &[uniform, binding]: shader->uniformToBinding) {
			this->uniformToBinding[uniform] = binding;
		}
	}
	#else
	if(this->window->backend == OPENGL_BACKEND) {
		if(this->program == GL_INVALID_INDEX) {
			GLuint program = glCreateProgram();
			
			for(Shader* shader: this->shaders) {
				if(shader->shader == GL_INVALID_INDEX) {
					console::error("shaders not compiled\n");
					return;
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
			}
			else {
				this->program = program;
			}

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

		glUseProgram(this->program);
	}
	#endif
}

void render::Program::bindUniform(std::string uniformName, void* data, unsigned int size, uint64_t cacheIndex, bool setOnce) {
	#ifdef __switch__
	if(this->uniformToPiece.find(uniformName) == this->uniformToPiece.end()) {
		this->createUniformMemory(uniformName, size);
	}

	this->window->commandBuffer.pushConstants(
		this->uniformToPiece[uniformName]->gpuAddr(),
		this->uniformToPiece[uniformName]->size(),
		0,
		size,
		data
	);

	// look for binding
	for(Shader* shader: this->shaders) {
		if(shader->uniformToBinding.find(uniformName) != shader->uniformToBinding.end()) {
			this->window->commandBuffer.bindUniformBuffer(
				shader->type == SHADER_FRAGMENT ? DkStage_Fragment : DkStage_Vertex,
				shader->uniformToBinding[uniformName],
				this->uniformToPiece[uniformName]->gpuAddr(),
				this->uniformToPiece[uniformName]->size()
			);
			break;
		}
	}
	#else
	if(this->window->backend == OPENGL_BACKEND) {
		auto found = this->uniformToBuffer.find(std::pair<std::string, uint64_t>(uniformName, cacheIndex));
		bool created = false;
		if(found == this->uniformToBuffer.end()) {
			this->createUniformBuffer(uniformName, size, cacheIndex);
			found = this->uniformToBuffer.find(std::pair<std::string, uint64_t>(uniformName, cacheIndex));
			created = true;
		}

		glBindBuffer(GL_UNIFORM_BUFFER, found.value());

		if(created || !setOnce) { // only update the buffer if we really need to
			// glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW); // orphan the buffer
			glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
		}
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

#ifdef __switch__
void render::Program::createUniformMemory(string uniformName, unsigned int size) {
	this->uniformToPiece[uniformName] = this->window->memory.allocate(
		DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, size, DK_UNIFORM_BUF_ALIGNMENT
	);
}
#else
void render::Program::createUniformBuffer(std::string uniformName, unsigned int size, uint64_t cacheIndex) {
	if(this->window->backend == OPENGL_BACKEND) {
		GLuint bufferId;
		glGenBuffers(1, &bufferId);
		this->uniformToBuffer[std::pair<std::string, uint64_t>(uniformName, cacheIndex)] = bufferId;

		glBindBuffer(GL_UNIFORM_BUFFER, this->uniformToBuffer[std::pair<std::string, uint64_t>(uniformName, cacheIndex)]);
		glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, this->uniformToBinding.find(uniformName).value(), bufferId);
	}
}
#endif
