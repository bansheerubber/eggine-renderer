#ifdef __switch__
#include "memory.h"

#include <string>

#include "../util/align.h"
#include "../engine/console.h"
#include "window.h"

render::switch_memory::Page* TEST_PAGE = nullptr;

render::switch_memory::Piece::Piece(Page* parent) {
	this->parent = parent;
}

void render::switch_memory::Piece::print() {
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

uint64_t render::switch_memory::Piece::size() {
	return this->end - this->start + 1;
}

void* render::switch_memory::Piece::cpuAddr() {
	return (char*)this->parent->cpuAddr() + this->start;
}

DkGpuAddr render::switch_memory::Piece::gpuAddr() {
	return this->parent->gpuAddr() + this->start;
}

void render::switch_memory::Piece::requestDeallocate() {
	this->parent->deallocationList.push_back(this);
}

void render::switch_memory::Piece::deallocate() {
	this->parent->deallocate(this);
}

render::switch_memory::Page::Page(Window* window, uint32_t flags, uint64_t size) {
	size = alignTo(size, DK_MEMBLOCK_ALIGNMENT); // align to 4KB otherwise we'll crash
	
	this->window = window;
	this->size = size;
	this->head = new Piece(this);
	this->head->start = 0;
	this->head->end = size - 1;
	this->head->align = DK_MEMBLOCK_ALIGNMENT;
	this->flags = flags;

	this->block = dk::MemBlockMaker{this->window->device, (uint32_t)this->size}.setFlags(flags).create();
}

void* render::switch_memory::Page::cpuAddr() {
	return this->block.getCpuAddr();
}

DkGpuAddr render::switch_memory::Page::gpuAddr() {
	return this->block.getGpuAddr();
}

render::switch_memory::Piece* render::switch_memory::Page::allocate(uint64_t size, uint64_t align) {
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

void render::switch_memory::Page::processDeallocationList() {
	for(Piece* piece: this->deallocationList) {
		piece->deallocate();
	}
	this->deallocationList.clear();
}

void render::switch_memory::Page::deallocate(Piece* piece) {
	piece->allocated = false;
	this->combinePieces();
}

void render::switch_memory::Page::combinePieces() {
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

void render::switch_memory::Page::print() {
	string flags = "";

	string enumNames[11] = { "DkMemBlockFlags_CpuAccessShift", "DkMemBlockFlags_GpuAccessShift", "DkMemBlockFlags_CpuUncached", "DkMemBlockFlags_CpuCached", "DkMemBlockFlags_GpuUncached", "DkMemBlockFlags_GpuCached", "DkMemBlockFlags_Code", "DkMemBlockFlags_Image", "DkMemBlockFlags_ZeroFillInit" };
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

	console::print("Page %p (\n%s) {\n", this, flags.c_str());
	Piece* current = this->head;
	while(current != nullptr) {
		current->print();
		current = current->next;
	}
	console::print("};\n\n");
}

render::switch_memory::Manager::Manager(Window* window) {
	this->window = window;
}

void render::switch_memory::Manager::processDeallocationLists() {
	for(Page* page: this->pages) {
		if(page->deallocationList.size() > 0) {
			page->processDeallocationList();
		}
	}
}

render::switch_memory::Piece* render::switch_memory::Manager::allocate(uint32_t flags, uint64_t size, uint64_t align) {
	uint64_t realSize = alignTo(size, align);
	
	Piece* foundPiece = nullptr;
	for(Page* page: this->pages) {
		if(page->size >= realSize && page->flags == flags) {
			foundPiece = page->allocate(realSize, align);
			if(foundPiece) {
				return foundPiece;
			}
		}
	}

	// we always need memory if we ask for it. create a new page
	uint64_t blockSize = realSize > SWITCH_MEMORY_DEFAULT_PAGE_SIZE ? realSize : SWITCH_MEMORY_DEFAULT_PAGE_SIZE;
	this->pages.emplace_back(new Page(this->window, flags, blockSize));
	this->allocated += blockSize;
	return this->pages.back()->allocate(realSize, align);
}

void render::switch_memory::Manager::print() {
	for(Page* page: this->pages) {
		page->print();
	}
	console::print("allocated: %ld\n", this->allocated);
}

uint64_t render::switch_memory::Manager::getAllocated() {
	return this->allocated;
}
#endif
