#include "renderer/window.h"

int main(int argc, char* argv[]) {
	render::Window window;
	window.initialize();

	for(unsigned int i = 0; i < 100; i++) {
		window.prerender();
		window.render();
	}

	window.deinitialize();
	
	return 0;
}
