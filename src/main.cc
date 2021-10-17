#ifndef __switch__
#include <glad/gl.h>
#endif

#include <array>
#include <glm/vec3.hpp>
#include <stdio.h>

#include "renderer/program.h"
#include "renderer/shader.h"
#include "renderer/vertexAttributes.h"
#include "renderer/vertexBuffer.h"
#include "renderer/window.h"

#include "renderer/memory.h"

#ifdef __switch__
#include <switch.h>
#endif

int main(int argc, char* argv[]) {
	#ifdef __switch__
	Result res = romfsInit();
	if(R_FAILED(res)) {
		return 0;
	}
	#endif
	
	render::Window window;
	window.initialize();

	glm::vec3 triangleVertices[] = {
		glm::vec3(0, 1, 0),
		glm::vec3(-1, -1, 0),
		glm::vec3(1, -1, 0),
	};

	render::VertexBuffer triangleBuffer(&window);
	triangleBuffer.setData(triangleVertices, sizeof(triangleVertices), alignof(glm::vec3));

	glm::vec4 triangleColors[] = {
		glm::vec4(1, 0, 0, 1),
		glm::vec4(0, 1, 0, 1),
		glm::vec4(0, 0, 1, 1),
	};

	render::VertexBuffer triangleColorBuffer(&window);
	triangleColorBuffer.setData(triangleColors, sizeof(triangleColors), alignof(glm::vec4));

	render::Shader vertexShader(&window);
	vertexShader.loadFromFile("romfs:/basic_vsh.dksh", render::SHADER_VERTEX);

	render::Shader fragmentShader(&window);
	fragmentShader.loadFromFile("romfs:/color_fsh.dksh", render::SHADER_FRAGMENT);

	render::Program program(&window);
	program.addShader(&vertexShader);
	program.addShader(&fragmentShader);

	// window.memory.print();

	render::VertexAttributes triangleAttributes(&window);
	triangleAttributes.addVertexAttribute(&triangleBuffer, 0, 3, render::VERTEX_ATTRIB_FLOAT, 0, sizeof(glm::vec3), 0);
	triangleAttributes.addVertexAttribute(&triangleColorBuffer, 1, 4, render::VERTEX_ATTRIB_FLOAT, 0, sizeof(glm::vec4), 0);

	program.bind();
	triangleAttributes.bind();
	window.draw(render::PRIMITIVE_TRIANGLES, 0, 3, 0, 1);
	window.commandList = window.commandBuffer.finishList();

	for(unsigned int i = 0; i < 500; i++) {
		window.prerender();
		window.render();
	}

	#ifdef __switch__
	romfsExit();
	#endif

	window.deinitialize();
	
	return 0;
}
