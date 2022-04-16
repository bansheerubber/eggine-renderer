#include "getShaderSource.h"

#include <fstream>

#include "../engine/console.h"
#include "shaderSource.h"
#include "../renderer/window.h"

resources::ShaderSource* getShaderSource(render::Window* window, std::string fileName) {
	bool binary = false;
	#ifdef __switch__
	fileName += ".dksh";
	binary = true;
	#endif
	
	if(window->backend == render::VULKAN_BACKEND) {
		fileName += ".spv";
		binary = true;
	}

	// oh boy
	#ifndef __switch__
	std::ifstream file(fileName);
	if(file.bad() || file.fail()) {
		console::error("couldn't load shader\n");
		return nullptr;
  }

	file.seekg(0, file.end);
	uint64_t length = file.tellg();
	file.seekg(0, file.beg);
	char* buffer = new char[length];
	file.read((char*)buffer, length);
	file.close();

	resources::ShaderSource* source = new resources::ShaderSource((const unsigned char*)buffer, length);
	delete[] buffer;

	if(binary) {
		std::ifstream file2(fileName.replace(fileName.size() - 4, fileName.size(), ""));
		if(file2.bad() || file2.fail()) {
			console::error("couldn't load original\n");
			return nullptr;
		}

		file2.seekg(0, file2.end);
		uint64_t length2 = file2.tellg();
		file2.seekg(0, file2.beg);
		char* buffer2 = new char[length2];
		file2.read((char*)buffer2, length2);
		file2.close();

		source->original = new resources::ShaderSource((const unsigned char*)buffer2, length);
		delete[] buffer2;
	}

	return source;
	#endif

	return nullptr;
}
