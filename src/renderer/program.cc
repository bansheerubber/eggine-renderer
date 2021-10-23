#ifndef __switch__
#include <glad/gl.h>
#endif

#include "program.h"

#include "shader.h"
#include "window.h"

render::Program::Program(Window* window) {
	this->window = window;
}

void render::Program::addShader(Shader* shader) {
	this->shaders.push_back(shader);
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
	if(this->program == GL_INVALID_INDEX) {
		GLuint program = glCreateProgram();
		
		for(Shader* shader: this->shaders) {
			if(shader->shader == GL_INVALID_INDEX) {
				printf("shaders not compiled\n");
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
			printf("failed to link program:\n%.*s\n", logLength, log);

			this->program = GL_INVALID_INDEX - 1;
		}
		else {
			this->program = program;
		}

		int uniformCount = 0;
		for(Shader* shader: this->shaders) {
			glDetachShader(program, shader->shader);

			// handle uniform block bindings (reconcile differences between deko3d and opengl)
			for(auto &[uniform, binding]: shader->uniformToBinding) {
				// update shader binding for opengl
				GLuint blockIndex = glGetUniformBlockIndex(this->program, uniform.c_str());
				if(blockIndex != GL_INVALID_INDEX) {
					glUniformBlockBinding(this->program, blockIndex, binding + uniformCount);
					this->uniformToBinding[uniform] = binding + uniformCount;
				}
			}
			uniformCount += shader->uniformToBinding.size();
		}
	}

	glUseProgram(this->program);
	#endif
}

void render::Program::bindUniform(string uniformName, void* data, unsigned int size) {
	#ifdef __switch__
	#else
	if(this->uniformToBuffer.find(uniformName) == this->uniformToBuffer.end()) {
		this->createUniformBuffer(uniformName, size);
	}

	glBindBuffer(GL_UNIFORM_BUFFER, this->uniformToBuffer[uniformName]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
	glBindBufferBase(GL_UNIFORM_BUFFER, this->uniformToBinding[uniformName], this->uniformToBuffer[uniformName]);
	#endif
}

void render::Program::bindTexture(string uniformName, unsigned int texture) {
	#ifdef __switch__
	this->window->commandBuffer.bindTextures(DkStage_Fragment, 0, dkMakeTextureHandle(0, 0));
	#else
	// get the location of the uniform we're going to bind to
	GLuint location = glGetUniformLocation(this->program, uniformName.c_str());
	glUniform1i(location, texture);
	#endif
}

#ifndef __switch__
void render::Program::createUniformBuffer(string uniformName, unsigned int size) {
	GLuint bufferId;
	glGenBuffers(1, &bufferId);
	this->uniformToBuffer[uniformName] = bufferId;

	glBindBuffer(GL_UNIFORM_BUFFER, this->uniformToBuffer[uniformName]);
	glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
}
#endif
