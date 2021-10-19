#pragma once

inline unsigned long alignTo(unsigned long size, unsigned long align) {
	return (size + align - 1) & ~(align - 1);
}
