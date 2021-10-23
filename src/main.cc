#ifndef __switch__
#include <glad/gl.h>
#endif

#include <array>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <stdio.h>

#include "renderer/program.h"
#include "renderer/shader.h"
#include "renderer/texture.h"
#include "renderer/vertexAttributes.h"
#include "renderer/vertexBuffer.h"
#include "renderer/window.h"

#include "util/align.h"

#include "renderer/memory.h"

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

	string filePrefix = "romfs/";
	#ifdef __switch__
	filePrefix = "romfs:/";
	#endif
	
	render::Window window;
	window.initialize();

	glm::vec2 triangleVertices[] = {
		glm::vec2(-0.5, 1.0),
		glm::vec2(-0.5, -1.0),
		glm::vec2(0.5, 1.0),
		glm::vec2(0.5, -1.0),
	};

	render::VertexBuffer triangleBuffer(&window);
	triangleBuffer.setData(triangleVertices, sizeof(triangleVertices), alignof(glm::vec2));

	glm::vec2 triangleUVs[] = {
		glm::vec2(0.0, 0.0),
		glm::vec2(0.0, 1.0),
		glm::vec2(1.0, 0.0),
		glm::vec2(1.0, 1.0),
	};

	render::VertexBuffer triangleColorBuffer(&window);
	triangleColorBuffer.setData(triangleUVs, sizeof(triangleUVs), alignof(glm::vec2));

	render::Texture texture(&window);
	texture.setWrap(render::TEXTURE_WRAP_CLAMP_TO_BORDER, render::TEXTURE_WRAP_CLAMP_TO_BORDER);
	texture.setFilters(render::TEXTURE_FILTER_NEAREST, render::TEXTURE_FILTER_NEAREST);
	texture.loadPNGFromFile(filePrefix + "spritesheet.png");

	render::Shader vertexShader(&window);
	vertexShader.loadFromFile(filePrefix + "basic.vert", render::SHADER_VERTEX);

	render::Shader fragmentShader(&window);
	fragmentShader.loadFromFile(filePrefix + "color.frag", render::SHADER_FRAGMENT);

	render::Program program(&window);
	program.addShader(&vertexShader);
	program.addShader(&fragmentShader);

	render::VertexAttributes triangleAttributes(&window);
	triangleAttributes.addVertexAttribute(&triangleBuffer, 0, 2, render::VERTEX_ATTRIB_FLOAT, 0, sizeof(glm::vec2), 0);
	triangleAttributes.addVertexAttribute(&triangleColorBuffer, 1, 2, render::VERTEX_ATTRIB_FLOAT, 0, sizeof(glm::vec2), 0);

	#ifdef __switch__
	printf("%ld bytes allocated\n", window.memory.getAllocated());
	#endif

	TestUniformBlock block = {
		color: glm::vec4(1, 0, 0, 1),
	};

	if(!window.getErrorCount()) {
		for(unsigned int i = 0; i < 500; i++) {
			window.prerender();

			program.bind();
			texture.bind(0);
			program.bindTexture("inTexture", 0);
			triangleAttributes.bind();

			block.color.r = abs(cos((double)i / 10));

			program.bindUniform("fragment", &block, sizeof(block));

			window.draw(render::PRIMITIVE_TRIANGLE_STRIP, 0, 4, 0, 1);

			window.render();
		}
	}

	#ifdef __switch__
	romfsExit();
	#endif

	window.deinitialize();
	
	return 0;
}
