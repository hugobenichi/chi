#include <base.h>

#include <stdio.h>

void* debug_malloc(const char* loc, const char* func, size_t size)
{
	printf("%s %s: malloc(%lu)\n", loc, func, size);
	return malloc(size);
}

void* debug_realloc(const char* loc, const char* func, void* ptr, size_t size)
{
	printf("%s %s: realloc(%p, %lu)\n", loc, func, ptr, size);
	return realloc(ptr, size);
}

size_t array_find_capacity(size_t current, size_t needed)
{
	static unsigned int capacity_scaling = 4;
	static unsigned int capacity_min = 16;
	if (current == 0)
		current = capacity_min;
	while (current <= needed)
		current = current * capacity_scaling;
	return current;
}

