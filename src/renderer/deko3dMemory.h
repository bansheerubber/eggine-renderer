#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#include <stdio.h>
#include <vector>

namespace render {
	class Window;
	
	namespace switch_memory {
		#define SWITCH_MEMORY_DEFAULT_PAGE_SIZE 4 * 1024
		
		// linked list structure that keeps track of free memory regions
		class Piece {
			friend class Page;
			
			public:
				Piece(class Page* parent);
				
				uint64_t start; // start of the chunk relative to the memory block
				uint64_t end; // end of the chunk relative to the memory block
				uint64_t align;
				Page* parent = nullptr;

				void print();
				uint64_t size();
				void* cpuAddr();
				DkGpuAddr gpuAddr();
				void requestDeallocate();
				void deallocate();

			protected:
				Piece* next = nullptr;
				Piece* prev = nullptr;
				bool allocated = false;
		};

		// structure to handle a MemBlock and split it up into usable pieces for data
		class Page {
			friend class Manager;
			friend Piece;
			
			public:
				Page(Window* window, uint32_t flags, uint64_t size = SWITCH_MEMORY_DEFAULT_PAGE_SIZE);

				dk::MemBlock block;
				
				Piece* allocate(uint64_t size, uint64_t align);
				void processDeallocationList();
				void deallocate(Piece* piece);
				void print();

				void* cpuAddr();
				DkGpuAddr gpuAddr();
			
			protected:
				Window* window;
				Piece* head = nullptr;
				uint64_t size;
				uint32_t flags;
				std::vector<Piece*> deallocationList;

				void combinePieces();
		};

		// handles all switch memory
		class Manager {
			public:
				Manager(Window* window);
				
				Piece* allocate(uint32_t flags, uint64_t size, uint64_t align);
				void processDeallocationLists();
				void print();
				uint64_t getAllocated();
			
			protected:
				std::vector<Page*> pages;
				Window* window;
				uint64_t allocated = 0;
		};
	};
};

#endif
