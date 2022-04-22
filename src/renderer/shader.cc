#ifndef __switch__
#include <glad/gl.h>
#endif

#include "shader.h"

#include <fstream>
#include <string.h>

#include "../engine/console.h"
#include "window.h"

#ifdef __switch__
struct DkshHeader {
	uint32_t magic; // DKSH_MAGIC
	uint32_t headerSize; // sizeof(DkshHeader)
	uint32_t controlSize;
	uint32_t codeSize;
	uint32_t programsOffset;
	uint32_t programCount;
};
#endif

render::Shader::Shader(Window* window) {
	this->window = window;
}

void render::Shader::bind() {
	
}

void render::Shader::load(resources::ShaderSource* source, ShaderType type) {
	if(source == nullptr) {
		console::error("shader source is nullptr\n");
		return;
	}

	this->type = type;

	if(source->original != nullptr) {
		this->processUniforms((const char*)source->original->buffer, source->original->bufferSize);	
	}
	else {
		this->processUniforms((const char*)source->buffer, source->bufferSize);
	}
	
	#ifdef __switch__
	const char* buffer = (const char*)source->buffer;

	DkshHeader header {
		magic: 0,
		headerSize: 0,
		controlSize: 0,
		codeSize: 0,
		programsOffset: 0,
		programCount: 0,
	};
	memcpy(&header, buffer, sizeof(header));

	if(header.magic != 0x48534b44) {
		console::error("couldn't load dksh\n");
		return;
	}

	std::vector<char> controlBuffer(header.controlSize);
	memcpy(controlBuffer.data(), buffer, header.controlSize);

	this->memory = this->window->memory.allocate(
		DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code,
		header.codeSize,
		DK_SHADER_CODE_ALIGNMENT
	);

	memcpy(this->memory->cpuAddr(), &buffer[header.controlSize], header.codeSize); // read code straight into code memory

	dk::ShaderMaker{this->memory->parent->block, (uint32_t)this->memory->start}
		.setControl(controlBuffer.data())
		.setProgramId(0)
		.initialize(this->shader);
	
	if(!this->shader.isValid()) {
		console::error("shader not valid\n");
		return;
	}
	#else
	const char* buffer = (const char*)source->buffer;
	uint64_t length = source->bufferSize;

	if(this->window->backend == OPENGL_BACKEND) {
		GLenum glType = type == SHADER_FRAGMENT ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER;

		GLuint shader = glCreateShader(glType);
		int glLength = length;
		glShaderSource(shader, 1, &buffer, &glLength);
		glCompileShader(shader);

		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if(compiled == GL_FALSE) {
			// print the error log
			GLint logLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

			GLchar* log = new GLchar[logLength];
			glGetShaderInfoLog(shader, logLength, &logLength, log);

			glDeleteShader(shader);

			console::error("failed to compile shader:\n%.*s\n", logLength, log);
		}
		else {
			this->shader = shader;
		}
	}
	else {
		std::vector<char> data;
		for(uint64_t i = 0; i < length; i++) {
			data.push_back(buffer[i]);
		}
		vk::ShaderModuleCreateInfo createInfo(
			{}, data.size(), reinterpret_cast<const uint32_t*>(data.data())
		);

		this->module = this->window->device.device.createShaderModule(createInfo, nullptr);
	}
	#endif
}

void render::Shader::processUniforms(const char* buffer, uint64_t bufferSize) {
	std::string line;
	uint64_t index = 0;
	while(index < bufferSize) {
		// read a line
		line = "";
		for(; buffer[index] != '\n' && index < bufferSize; index++) {
			line += buffer[index];
		}
		index++; // skip the newline

		uint64_t uniformLocation = line.find("uniform");
		if(uniformLocation != std::string::npos) {
			uint64_t bindingLocation = line.find("binding");
			if(bindingLocation == std::string::npos) {
				console::error("could not find binding for uniform\n");
				return;
			}

			std::string buffer;
			for(unsigned int i = bindingLocation; i < line.length(); i++) {
				switch(line[i]) {
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9': {
						buffer += line[i];
						break;
					}

					case ',':
					case ')': {
						goto end;
					}
				}
			}
			
			end:
			unsigned int binding = std::stod(buffer);
			buffer = "";
			bool sampler = false;
			for(unsigned int i = uniformLocation + std::string("uniform ").length(); i < line.length(); i++) {
				if(line[i] == ' ') {
					if(buffer == "sampler2D") {
						sampler = true;
					}
					buffer = "";
				}
				else if(
					(line[i] >= 'a' && line[i] <= 'z')
					|| (line[i] >= 'A' && line[i] <= 'Z')
					|| (line[i] >= '0' && line[i] <= '9')
					|| line[i] == '_'
				) {
					buffer += line[i];
				}
			}
			this->uniformToBinding[buffer] = binding;
			this->isUniformSampler[buffer] = sampler;
		}
	}
}
