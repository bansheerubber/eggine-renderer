#define __switch__

#include <array>
#include <glm/vec3.hpp>
#include <stdio.h>

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
	vertexShader.load("romfs:/basic_vsh.dksh");

	render::Shader fragmentShader(&window);
	fragmentShader.load("romfs:/color_fsh.dksh");

	window.memory.print();

	render::VertexAttributes triangleAttributes(&window);
	triangleAttributes.addVertexAttribute(&triangleBuffer, 0, 3, 32, 0, sizeof(glm::vec3), 0);

	std::array<DkShader const*, 2> shaderArray = { &vertexShader.shader, &fragmentShader.shader };

	window.commandBuffer.bindShaders(DkStageFlag_GraphicsMask, shaderArray);
	triangleAttributes.bind();
	window.commandBuffer.draw(DkPrimitive_Triangles, 3, 1, 0, 0);

	triangleBuffer.bind(0);

	window.commandList = window.commandBuffer.finishList();

	for(unsigned int i = 0; i < 200; i++) {
		window.prerender();
		window.render();
	}

	#ifdef __switch__
	romfsExit();
	#endif

	window.deinitialize();
	
	return 0;
}
