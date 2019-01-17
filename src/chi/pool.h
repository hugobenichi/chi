#ifndef __chi_pool__
#define __chi_pool__

#include <chi/base.h>
#include <chi/memory.h>


// A pool of constant size items.
// - cannot be resized and will not move objects inside.
// - overhead of 24B + 4B per item.
// - constant time alloc object and return object.
struct pool {
	size_t object_size;
	int capacity;
	int used;
  int next_free_object;
  int last_free_object;
  int free_object_map[0];
};

#define pool_is_full(pool) (pool->used == pool->capacity)

struct pool* pool_init(size_t object_size, int capacity);
int pool_get_index(struct pool *pool, void* object);
void* pool_get_object(struct pool *pool, int object_indext);
void* pool_new_object(struct pool *pool);
void pool_return_object(struct pool *pool, void* object);

#endif
