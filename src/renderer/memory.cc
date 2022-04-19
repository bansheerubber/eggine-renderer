#include "memory.h"

#include <string>

#include "../util/align.h"
#include "../engine/console.h"
#include "window.h"

render::Page* TEST_PAGE = nullptr;

render::Piece::Piece(Page* parent) {
	this->parent = parent;
}

void render::Piece::print() {
	console::print("  Piece %p {\n", this);
	console::print("    allocated: %d,\n", this->allocated);
	console::print("    next: %p,\n", this->next);
	console::print("    prev: %p,\n", this->prev);
	console::print("    start: %ld,\n", this->start);
	console::print("    end: %ld,\n", this->end);
	console::print("    align: %ld,\n", this->align);
	console::print("    size: %ld,\n", this->end - this->start + 1);
	console::print("  };\n");
}

uint64_t render::Piece::size() {
	return this->end - this->start + 1;
}

#ifdef __switch__
void* render::Piece::cpuAddr() {
	return (char*)this->parent->cpuAddr() + this->start;
}

DkGpuAddr render::Piece::gpuAddr() {
	return this->parent->gpuAddr() + this->start;
}
#else
void* render::Piece::map() {
	return (char*)this->parent->data + this->start;
}

vk::Buffer render::Piece::getBuffer() {
	return this->buffer;
}
#endif

void render::Piece::requestDeallocate() {
	this->parent->deallocationList.push_back(this);
}

void render::Piece::deallocate() {
	this->parent->deallocate(this);

	#ifndef __switch__
	this->parent->window->device.device.destroyBuffer(this->buffer);
	this->valid = false;
	#endif
}

#ifdef __switch__
render::Page::Page(Window* window, uint32_t flags, uint64_t size) {
#else
render::Page::Page(Window* window, vk::MemoryPropertyFlags flags, uint32_t memoryTypeIndex, uint64_t size) {
#endif
	#ifdef __switch__
	size = alignTo(size, DK_MEMBLOCK_ALIGNMENT); // align to 4KB otherwise we'll crash
	#endif

	this->window = window;
	this->size = size;
	this->head = new Piece(this);
	this->head->start = 0;
	this->head->end = size - 1;
	#ifdef __switch__
	this->head->align = DK_MEMBLOCK_ALIGNMENT;
	#else
	this->head->align = 0;
	this->memoryTypeIndex = memoryTypeIndex;
	#endif
	this->flags = flags;

	#ifdef __switch__
	this->block = dk::MemBlockMaker{this->window->device, (uint32_t)this->size}.setFlags(flags).create();
	#else
	vk::MemoryAllocateInfo allocationInfo(size, memoryTypeIndex);
	this->memory = this->window->device.device.allocateMemory(allocationInfo);

	if(flags & vk::MemoryPropertyFlagBits::eHostVisible) {
		this->data = this->window->device.device.mapMemory(this->memory, 0, this->size, {});
	}
	#endif
}

#ifdef __switch__
void* render::Page::cpuAddr() {
	return this->block.getCpuAddr();
}

DkGpuAddr render::Page::gpuAddr() {
	return this->block.getGpuAddr();
}
#endif

render::Piece* render::Page::allocate(uint64_t size, uint64_t align) {
	uint64_t realSize = alignTo(size, align);

	if(realSize > this->size) {
		return nullptr;
	}

	// look through the pieces and see if there's a piece that is big enough to hold our memory
	Piece* current = this->head;
	while(current != nullptr) {
		uint64_t realStart = alignTo(current->start, align); // we need to align the start
		int64_t pieceSize = (int64_t)current->size() - (int64_t)(realStart - current->start); // calculate useable size using alignment

		if(pieceSize >= (int64_t)realSize && !current->allocated) { // we found a piece that we can use
			break;
		}
		
		current = current->next;
	}

	if(current == nullptr) {
		return nullptr;
	}
	else if(realSize != this->size) { // segment memory if needed
		uint64_t realStart = alignTo(current->start, align); // round up to closest alignment

		Piece* unusedSegment = new Piece(this);
		unusedSegment->start = realStart + realSize;
		unusedSegment->end = current->end;
		unusedSegment->align = 0;

		// add the unused alignment before the current element
		if(realStart != current->start) {
			Piece* unusedSegmentAlign = new Piece(this);
			unusedSegmentAlign->start = current->start;
			unusedSegmentAlign->end = current->start + (realStart - current->start - 1);
			unusedSegmentAlign->allocated = false;
			unusedSegmentAlign->align = 0;
			Piece* prev = current->prev;
			if(prev != nullptr) {
				prev->next = unusedSegmentAlign;
			}

			unusedSegmentAlign->prev = prev;
			unusedSegmentAlign->next = current;

			current->prev = unusedSegmentAlign;
		}

		// adjust current's size
		current->start = realStart;
		current->end = realStart + realSize - 1;
		current->align = align;
		current->allocated = true;

		// update the linked list
		Piece* oldNext = current->next;
		current->next = unusedSegment;

		if(oldNext != nullptr) {
			oldNext->prev = unusedSegment;
		}

		unusedSegment->next = oldNext;
		unusedSegment->prev = current;

		this->combinePieces();
		return current;
	}
	else { // we're using up the entire page
		current->allocated = true;
		return current;
	}
}

void render::Page::processDeallocationList() {
	for(Piece* piece: this->deallocationList) {
		piece->deallocate();
	}
	this->deallocationList.clear();
}

void render::Page::deallocate(Piece* piece) {
	piece->allocated = false;
	this->combinePieces();
}

void render::Page::combinePieces() {
	Piece* last = this->head;
	Piece* current = this->head->next;
	while(current != nullptr) {
		// combine deallocated pieces
		if(!last->allocated && !current->allocated) {
			// last will absorb current
			last->end = current->end;
			Piece* oldNext = current->next;
			last->next = oldNext;

			if(oldNext != nullptr) {
				oldNext->prev = last;
			}

			delete current;

			current = oldNext;
		}
		else {
			last = current;
			current = current->next;
		}
	}
}

void render::Page::print() {
	std::string flags = "";

	#ifdef __switch__
	std::string enumNames[11] = { "DkMemBlockFlags_CpuAccessShift", "DkMemBlockFlags_GpuAccessShift", "DkMemBlockFlags_CpuUncached", "DkMemBlockFlags_CpuCached", "DkMemBlockFlags_GpuUncached", "DkMemBlockFlags_GpuCached", "DkMemBlockFlags_Code", "DkMemBlockFlags_Image", "DkMemBlockFlags_ZeroFillInit" };
	uint32_t enums[11] = { DkMemBlockFlags_CpuAccessShift, DkMemBlockFlags_GpuAccessShift, DkMemBlockFlags_CpuUncached, DkMemBlockFlags_CpuCached, DkMemBlockFlags_GpuUncached, DkMemBlockFlags_GpuCached, DkMemBlockFlags_Code, DkMemBlockFlags_Image, DkMemBlockFlags_ZeroFillInit };

	for(int i = 0; i < 11; i++) {
		if(this->flags & enums[i]) {
			if(flags.length() == 0) {
				flags = "  " + enumNames[i] + "\n";
			}
			else {
				flags += "  & " + enumNames[i] + "\n";
			}
		}
	}
	#endif

	console::print("Page %p (\n%s) {\n", this, flags.c_str());
	Piece* current = this->head;
	while(current != nullptr) {
		current->print();
		current = current->next;
	}
	console::print("};\n\n");
}

render::Manager::Manager(Window* window) {
	this->window = window;
}

void render::Manager::processDeallocationLists() {
	for(Page* page: this->pages) {
		if(page->deallocationList.size() > 0) {
			page->processDeallocationList();
		}
	}
}

#ifdef __switch__
render::Piece* render::Manager::allocate(uint32_t flags, uint64_t size, uint64_t align) {
#else
render::Piece* render::Manager::allocate(vk::BufferCreateInfo bufferInfo, vk::MemoryPropertyFlags propertyFlags) {
#endif
	#ifdef __switch__
	uint64_t realSize = alignTo(size, align);
	#else
	vk::Buffer buffer = this->window->device.device.createBuffer(bufferInfo);
	vk::MemoryRequirements requirements = this->window->device.device.getBufferMemoryRequirements(buffer);
	vk::DeviceSize align = requirements.alignment;
	
	uint64_t realSize = alignTo(requirements.size, requirements.alignment);
	uint32_t memoryTypeIndex = (uint32_t)-1;
	vk::PhysicalDeviceMemoryProperties properties = this->window->device.physicalDevice.getMemoryProperties();
	for(uint32_t i = 0; i < properties.memoryTypeCount; i++) {
		if((requirements.memoryTypeBits & (1 << i)) && (properties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
			memoryTypeIndex = i;
			break;
		}
	}

	if(memoryTypeIndex == (uint32_t)-1) {
		console::print("could not find memory type index\n");
		exit(1);
	}
	#endif
	
	Piece* foundPiece = nullptr;
	for(Page* page: this->pages) {
		#ifdef __switch__
		if(page->size >= realSize && page->flags == flags) {
		#else
		if(page->size >= realSize && page->flags == propertyFlags && page->memoryTypeIndex == memoryTypeIndex) {
		#endif
			foundPiece = page->allocate(realSize, align);
			#ifndef __switch__
			foundPiece->buffer = buffer;
			foundPiece->bufferSize = bufferInfo.size;
			foundPiece->valid = true;
			this->window->device.device.bindBufferMemory(buffer, foundPiece->parent->memory, foundPiece->start);
			#endif

			if(foundPiece) {
				return foundPiece;
			}
		}
	}

	// we always need memory if we ask for it. create a new page
	uint64_t blockSize = realSize > DEFAULT_PAGE_SIZE ? realSize : DEFAULT_PAGE_SIZE;
	#ifdef __switch__
	this->pages.emplace_back(new Page(this->window, flags, blockSize));
	#else
	this->pages.emplace_back(new Page(this->window, propertyFlags, memoryTypeIndex, blockSize));
	#endif
	this->allocated += blockSize;

	#ifdef __switch__
	return this->pages.back()->allocate(realSize, align);
	#else
	Piece* piece = this->pages.back()->allocate(realSize, align);
	piece->buffer = buffer;
	piece->bufferSize = bufferInfo.size;
	piece->valid = true;
	this->window->device.device.bindBufferMemory(buffer, piece->parent->memory, piece->start);

	return piece;
	#endif
}

void render::Manager::print() {
	for(Page* page: this->pages) {
		page->print();
	}
	console::print("allocated: %ld\n", this->allocated);
}

uint64_t render::Manager::getAllocated() {
	return this->allocated;
}
