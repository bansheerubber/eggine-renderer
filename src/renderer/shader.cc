#ifndef __switch__
#include <glad/gl.h>
#endif

#include "shader.h"

#include <fstream>
#include <string.h>

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

void render::Shader::loadFromFile(string filename, ShaderType type) {
	this->processUniforms(filename);
	
	#ifdef __switch__
	filename += ".dksh";
	#endif
	
	ifstream file(filename);

	if(file.bad() || file.fail()) {
		printf("failed to open file for png %s\n", filename.c_str());
		file.close();
		this->window->addError();
		return;
  }

	file.seekg(0, file.end);
	unsigned long length = file.tellg();
	file.seekg(0, file.beg);
	char* buffer = new char[length];
	file.read((char*)buffer, length);
	file.close();

	this->load(buffer, length, type);

	delete[] buffer;
}

void render::Shader::load(char* buffer, size_t length, ShaderType type) {
	this->type = type;
	#ifdef __switch__
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
		printf("couldn't load dksh\n");
		return;
	}

	vector<char> controlBuffer(header.controlSize);
	memcpy(controlBuffer.data(), buffer, header.controlSize);

	this->memory = this->window->memory.allocate(
		DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code,
		header.codeSize,
		DK_SHADER_CODE_ALIGNMENT
	);

	memcpy(this->memory->cpuAddr(), &buffer[header.controlSize], header.codeSize); // read code straight into code memory

	dk::ShaderMaker{this->memory->parent->block, this->memory->start}
		.setControl(controlBuffer.data())
		.setProgramId(0)
		.initialize(this->shader);
	
	if(!this->shader.isValid()) {
		printf("shader not valid\n");
		exit(1);
	}
	#else
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

		printf("failed to compile shader:\n%.*s\n", logLength, log);
	}
	else {
		this->shader = shader;
	}
	#endif
}

// find the uniforms and identify them so we can do some stuff magically
void render::Shader::processUniforms(string filename) {
	ifstream file(filename);
	if(file.bad() || file.fail()) {
		file.close();
		return;
	}

	string line;
	while(getline(file, line)) {
		size_t uniformLocation = line.find("uniform");
		if(uniformLocation != string::npos) {
			size_t bindingLocation = line.find("binding");
			if(bindingLocation == string::npos) {
				printf("could not find binding for uniform\n");
				file.close();
				return;
			}

			string buffer;
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
			unsigned int binding = stod(buffer);
			buffer = "";
			for(unsigned int i = uniformLocation + string("uniform ").length(); i < line.length(); i++) {
				if(line[i] == ' ') {
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
		}
	}
	file.close();
}
