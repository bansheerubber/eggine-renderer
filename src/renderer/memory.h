#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#include <stdio.h>
#include <vector>

using namespace std;

namespace render {
	class Window;
	
	namespace switch_memory {
		#define SWITCH_MEMORY_DEFAULT_PAGE_SIZE 4 * 1024
		
		// linked list structure that keeps track of free memory regions
		class Piece {
			friend class Page;
			
			public:
				Piece(class Page* parent);
				
				unsigned long start; // start of the chunk relative to the memory block
				unsigned long end; // end of the chunk relative to the memory block
				Page* parent = nullptr;

				void print();
				unsigned long size();
				void* cpuAddr();
				DkGpuAddr gpuAddr();
				void deallocate();

			protected:
				Piece* next = nullptr;
				Piece* prev = nullptr;
				bool allocated = false;
		};

		// structure to handle a MemBlock and split it up into usable pieces for data
		class Page {
			friend class Manager;
			
			public:
				Page(Window* window, uint32_t flags, unsigned long size = SWITCH_MEMORY_DEFAULT_PAGE_SIZE);

				dk::MemBlock block;
				
				Piece* allocate(unsigned long size, unsigned long align);
				void deallocate(Piece* piece);
				void print();

				void* cpuAddr();
				DkGpuAddr gpuAddr();
			
			protected:
				Window* window;
				Piece* head = nullptr;
				unsigned long size;
				uint32_t flags;

				void combinePieces();
		};

		// handles all switch memory
		class Manager {
			public:
				Manager(Window* window);
				
				Piece* allocate(uint32_t flags, unsigned long size, unsigned long align);
				void print();
				size_t getAllocated();
			
			protected:
				vector<Page*> pages;
				Window* window;
				size_t allocated = 0;
		};
	};
};

#endif
