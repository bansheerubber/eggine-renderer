#pragma once

#include <cstdint>

inline uint64_t alignTo(uint64_t size, uint64_t align) {
	return (size + align - 1) & ~(align - 1);
}
