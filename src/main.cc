#include <glad/gl.h>

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

#include <string.h>

struct Vertex {
	glm::vec3 test1;
	glm::vec3 test2;
};

int main(int argc, char* argv[]) {
	printf("%ld\n", offsetof(Vertex, test1));

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

	render::Shader vertexShader(&window);
	vertexShader.loadFromFile("romfs/basic_vsh.glsl", render::SHADER_VERTEX);

	render::Shader fragmentShader(&window);
	fragmentShader.loadFromFile("romfs/color_fsh.glsl", render::SHADER_FRAGMENT);

	render::Program program(&window);
	program.addShader(&vertexShader);
	program.addShader(&fragmentShader);

	// window.memory.print();

	render::VertexAttributes triangleAttributes(&window);
	triangleAttributes.addVertexAttribute(&triangleBuffer, 0, 3, 32, 0, sizeof(glm::vec3), 0);

	// window.commandBuffer.draw(DkPrimitive_Triangles, 3, 1, 0, 0);

	// window.commandList = window.commandBuffer.finishList();

	for(unsigned int i = 0; i < 500; i++) {
		window.prerender();

		program.bind();
		triangleAttributes.bind();
		glDrawArrays(GL_TRIANGLES, 0, 3);

		window.render();
	}

	#ifdef __switch__
	romfsExit();
	#endif

	window.deinitialize();
	
	return 0;
}
