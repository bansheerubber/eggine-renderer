#ifndef __switch__
#include <glad/gl.h>
#endif

#include <array>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <stdio.h>

#include "renderer/program.h"
#include "renderer/shader.h"
#include "renderer/texture.h"
#include "renderer/vertexAttributes.h"
#include "renderer/vertexBuffer.h"
#include "renderer/window.h"

#include "renderer/memory.h"

#include "resources/getShaderSource.h"

#ifdef __switch__
#include <switch.h>
#endif

struct TestUniformBlock {
	glm::vec4 color;
};

int main(int argc, char* argv[]) {
	#ifdef __switch__
	Result res = romfsInit();
	if(R_FAILED(res)) {
		return 0;
	}
	#endif

	std::string filePrefix = "romfs/";
	#ifdef __switch__
	filePrefix = "romfs:/";
	#endif
	
	render::Window window;
	window.initialize();

	#ifdef __switch__
	printf("%ld bytes allocated\n", window.memory.getAllocated());
	#endif

	// shaders
	render::Shader* vertexShader = new render::Shader(&window);
	vertexShader->load(getShaderSource(&window, filePrefix + "hello.vert"), render::SHADER_VERTEX);

	render::Shader* fragmentShader = new render::Shader(&window);
	fragmentShader->load(getShaderSource(&window, filePrefix + "hello.frag"), render::SHADER_FRAGMENT);

	// programs
	render::Program* simpleProgram = new render::Program(&window);
	simpleProgram->addShader(vertexShader);
	simpleProgram->addShader(fragmentShader);

	render::State state = window.getState(0);

	// vertices
	glm::vec2 triangleVertices[] = {
		glm::vec2(-0.5, 0.8),
		glm::vec2(-0.5, -1.0),
		glm::vec2(0.5, 0.8),
		glm::vec2(0.5, -1.0),
	};

	render::VertexBuffer triangleBuffer(&window);
	triangleBuffer.setData(triangleVertices, sizeof(triangleVertices), alignof(glm::vec2));

	glm::vec3 triangleColors[] = {
		glm::vec3(1.0, 0.0, 0.0),
		glm::vec3(0.0, 1.0, 0.0),
		glm::vec3(0.0, 0.0, 1.0),
		glm::vec3(1.0, 0.0, 1.0),
	};

	render::VertexBuffer colorBuffer(&window);
	colorBuffer.setData(triangleColors, sizeof(triangleColors), alignof(glm::vec3));

	render::VertexAttributes triangleAttributes(&window);
	triangleAttributes.addVertexAttribute(&triangleBuffer, 0, 2, render::VERTEX_ATTRIB_FLOAT, 0, sizeof(glm::vec2), 0);
	triangleAttributes.addVertexAttribute(&colorBuffer, 1, 3, render::VERTEX_ATTRIB_FLOAT, 0, sizeof(glm::vec3), 0);

	if(!window.getErrorCount()) {
		for(unsigned int i = 0; i < 100000; i++) {
			window.prerender();
			state.bindVertexAttributes(&triangleAttributes);
			state.bindProgram(simpleProgram);
			state.draw(render::PRIMITIVE_TRIANGLE_STRIP, 0, 4, 0, 1);
			window.render();
		}
	}

	#ifdef __switch__
	romfsExit();
	#endif

	window.deinitialize();
	
	return 0;
}
