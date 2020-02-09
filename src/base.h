#ifndef __chi_base__
#define __chi_base__

#include <stddef.h>
#include <stdlib.h>

// Various utilities
template <typename T>
void swap(T& a, T& b)
{
	T t = a;
	a = b;
	b = t;
}

// Memory
template <typename T>
T* mcopy(const T* src, size_t srclen)
{
	T* src_copy = (T*) malloc(srclen);
	memcpy(src_copy, src, srclen);
	return src_copy;
}
#if DEBUG
	#define realloc(ptr, size)  debug_realloc(__LOC__, __func__, ptr, size)
	#define malloc(size)        debug_malloc(__LOC__, __func__, size)
#endif
void* debug_malloc(const char* loc, const char* func, size_t size);
void* debug_realloc(const char* loc, const char* func, void* ptr, size_t size);

// Arrays and collections
size_t array_find_capacity(size_t current, size_t needed);

#endif //__chi_base__
