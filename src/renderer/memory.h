#pragma once

#ifdef __switch__
#include <deko3d.hpp>
#else
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#endif

#include <stdio.h>
#include <vector>

namespace render {
	class Window;
	
	#ifdef __switch__
	#define DEFAULT_PAGE_SIZE 4 * 1024
	#else
	#define DEFAULT_PAGE_SIZE 50 * 1024

	enum PieceType {
		INVALID_PIECE = 0,
		BUFFER_PIECE,
		IMAGE_PIECE,
	};
	#endif
	
	// linked list structure that keeps track of free memory regions
	class Piece {
		friend class Manager;
		friend class Page;
		friend class State;
		friend class Window;
		
		public:
			Piece(class Page* parent);
			
			uint64_t start; // start of the chunk relative to the memory block
			uint64_t end; // end of the chunk relative to the memory block
			uint64_t align;
			Page* parent = nullptr;

			void print();
			uint64_t size();

			#ifdef __switch__
			void* cpuAddr();
			DkGpuAddr gpuAddr();
			#else
			void* map();
			vk::Buffer getBuffer();
			vk::Image getImage();
			#endif

			void requestDeallocate();
			void deallocate();

		protected:
			Piece* next = nullptr;
			Piece* prev = nullptr;
			bool allocated = false;

			#ifndef __switch__
			vk::Buffer buffer;
			vk::Image image;
			PieceType valid = INVALID_PIECE;
			uint64_t bufferSize = 0;
			#endif
	};

	// structure to handle a MemBlock and split it up into usable pieces for data
	class Page {
		friend class Manager;
		friend Piece;
		
		public:
			#ifdef __switch__
			Page(Window* window, uint32_t flags, uint64_t size);
			#else
			Page(Window* window, vk::MemoryPropertyFlags flags, uint32_t memoryTypeIndex, uint64_t size);
			#endif

			#ifdef __switch__
			dk::MemBlock block;
			#else
			vk::DeviceMemory memory;
			#endif
			
			Piece* allocate(uint64_t size, uint64_t align);
			void processDeallocationList();
			void deallocate(Piece* piece);
			void print();

			#ifdef __switch__
			void* cpuAddr();
			DkGpuAddr gpuAddr();
			#endif
		
		protected:
			Window* window;
			Piece* head = nullptr;
			uint64_t size;
			#ifdef __switch__
			uint32_t flags;
			#else
			vk::MemoryPropertyFlags flags;
			uint32_t memoryTypeIndex;

			void* data = nullptr;
			#endif
			std::vector<Piece*> deallocationList;

			void combinePieces();
	};

	// handles all switch memory
	class Manager {
		public:
			Manager(Window* window);
			
			#ifdef __switch__
			Piece* allocate(uint32_t flags, uint64_t size, uint64_t align);
			#else
			Piece* allocateBuffer(vk::BufferCreateInfo bufferInfo, vk::MemoryPropertyFlags propertyFlags);
			Piece* allocateImage(vk::ImageCreateInfo imageInfo, vk::MemoryPropertyFlags propertyFlags);
			#endif

			void processDeallocationLists();
			void print();
			uint64_t getAllocated();
		
		protected:
			std::vector<Page*> pages;
			Window* window;
			uint64_t allocated = 0;

			#ifndef __switch__
			Piece* allocate(
				vk::MemoryRequirements requirements, vk::MemoryPropertyFlags propertyFlags, uint64_t size, uint64_t align
			);
			#endif
	};
};
