#include <fstream>
#include <string.h>

#include "shader.h"

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

void render::Shader::load(string filename) {
	#ifdef __switch__
	ifstream file(filename);

	file.seekg(0, file.end);
	unsigned long length = file.tellg();
	file.seekg(0, file.beg);

	DkshHeader header {
		magic: 0,
		headerSize: 0,
		controlSize: 0,
		codeSize: 0,
		programsOffset: 0,
		programCount: 0,
	};
	file.read((char*)&header, sizeof(header));

	if(header.magic != 0x48534b44) {
		printf("couldn't load dksh\n");
		return;
	}

	file.seekg(0, file.beg);
	vector<char> controlBuffer(header.controlSize);
	file.read(controlBuffer.data(), header.controlSize);

	this->memory = this->window->memory.allocate(
		DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code,
		header.codeSize,
		DK_SHADER_CODE_ALIGNMENT
	);

	file.read((char*)this->memory->cpuAddr(), header.codeSize); // read code straight into code memory

	file.close();

	dk::ShaderMaker{this->memory->parent->block, this->memory->start}
		.setControl(controlBuffer.data())
		.setProgramId(0)
		.initialize(this->shader);
	
	if(!this->shader.isValid()) {
		printf("shader not valid\n");
		exit(1);
	}
	#endif
}
