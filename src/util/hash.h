#pragma once

#include <functional>

template<class T>
inline size_t combineHash(size_t hash, const T &ob) {
	std::hash<T> hasher;
	return hash ^ (hasher(ob) + 0x9e3779b9 + (hash << 6) + (hash >> 2));
}
