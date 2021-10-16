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

		for(Shader* shader: this->shaders) {
			glDetachShader(program, shader->shader);
		}
	}

	glUseProgram(this->program);
	#endif
}
