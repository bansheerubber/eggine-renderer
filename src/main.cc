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

	socketInitializeDefault();
	int nxlink = nxlinkStdio();
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
		glm::vec2(-0.501f,  1.001f),
		glm::vec2(-0.501f, -1.001f),
		glm::vec2(0.501f, 1.001f),
		glm::vec2(0.501f, -1.001f),
	};

	render::VertexBuffer triangleBuffer(&window);
	triangleBuffer.setDynamicDraw(true);
	triangleBuffer.setData(triangleVertices, sizeof(triangleVertices), alignof(glm::vec2));

	// colors
	glm::vec3 triangleColors[] = {
		glm::vec3(1.0, 0.0, 0.0),
		glm::vec3(0.0, 1.0, 0.0),
		glm::vec3(0.0, 0.0, 1.0),
		glm::vec3(1.0, 0.0, 1.0),
	};

	render::VertexBuffer colorBuffer(&window);
	colorBuffer.setData(triangleColors, sizeof(triangleColors), alignof(glm::vec3));

	// uvs
	glm::vec2 triangleUVs[] = {
		glm::vec2(0.0f, 0.0f),
		glm::vec2(0.0f, 1.0),
		glm::vec2(1.0, 0.0f),
		glm::vec2(1.0, 1.0),
	};

	render::VertexBuffer uvBuffer(&window);
	uvBuffer.setData(triangleUVs, sizeof(triangleUVs), alignof(glm::vec2));

	render::VertexAttributes triangleAttributes(&window);
	triangleAttributes.addVertexAttribute(&triangleBuffer, 0, 2, render::VERTEX_ATTRIB_FLOAT, 0, sizeof(glm::vec2), 0);
	triangleAttributes.addVertexAttribute(&colorBuffer, 1, 3, render::VERTEX_ATTRIB_FLOAT, 0, sizeof(glm::vec3), 0);
	triangleAttributes.addVertexAttribute(&uvBuffer, 2, 2, render::VERTEX_ATTRIB_FLOAT, 0, sizeof(glm::vec2), 0);

	render::Texture texture(&window);
	texture.setWrap(render::TEXTURE_WRAP_CLAMP_TO_EDGE, render::TEXTURE_WRAP_CLAMP_TO_EDGE);
	texture.setFilters(render::TEXTURE_FILTER_LINEAR, render::TEXTURE_FILTER_LINEAR);
	texture.loadPNGFromFile(filePrefix + "spritesheet.png");

	if(!window.getErrorCount()) {
		for(unsigned int i = 0; i < 100000; i++) {
			window.prerender();

			glm::vec2 triangleVertices[] = {
				glm::vec2(-0.5, 0.8),
				glm::vec2(-0.5, -1.0),
				glm::vec2(0.5, 0.8),
				glm::vec2(0.5, -1.0),
			};

			for(uint8_t j = 0; j < 4; j++) {
				triangleVertices[j].x += static_cast<float>(rand()) / static_cast<float>(RAND_MAX) / 10.0f;
				triangleVertices[j].y += static_cast<float>(rand()) / static_cast<float>(RAND_MAX) / 10.0f;
			}	
			triangleBuffer.setData(triangleVertices, sizeof(triangleVertices), alignof(glm::vec2));

			state.bindProgram(simpleProgram);

			{
				struct VertexBlock {
					alignas(8) glm::vec2 offset;
				} vb;
				vb.offset = glm::vec2(0, 0.5);
				state.bindUniform("vertex", &vb, sizeof(vb));
			}

			{
				struct FragmentBlock {
					alignas(16) glm::vec4 color;
				} vb;
				vb.color = glm::vec4(0.2, 0.0, 0.2, 1);
				state.bindUniform("fragment", &vb, sizeof(vb));
			}

			state.bindTexture("inTexture", &texture);

			state.bindVertexAttributes(&triangleAttributes);
			state.draw(render::PRIMITIVE_TRIANGLE_STRIP, 0, 4, 0, 1);

			window.render();
		}
	}

	#ifdef __switch__
	romfsExit();
	close(nxlink);
	socketExit();
	#endif

	window.deinitialize();
	
	return 0;
}
