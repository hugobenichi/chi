#include <chi/pool.h>

#include <stdlib.h>

size_t pool_total_memory(size_t object_size, int capacity)
{
	return sizeof(struct pool) + capacity * (object_size * sizeof(int));
}

struct pool* pool_init(size_t object_size, int capacity)
{
	__assert1(object_size);
	__assert1(capacity);

	struct pool *pool = malloc(pool_total_memory(object_size, capacity));
	__assert1(pool);

	int last = capacity - 1;
	for (int i = 0; i < last; i++) {
		pool->free_object_map[i] = i+1;
	}
	pool->free_object_map[last] = last;

	pool->object_size = object_size;
	pool->capacity = capacity;
	pool->used = 0;
	pool->next_free_object = 0;
	pool->last_free_object = last;

	return pool;
}

int pool_get_index(struct pool *pool, void* object)
{
	u8 *base= (u8*) (pool->free_object_map + pool->capacity);
	u8 *obj = object;

	__assert1(base <= obj);
	__assert1(obj < base + pool->capacity * pool->object_size);

	return (obj - base) / pool->object_size;
}

void* pool_get_object(struct pool *pool, int object_index)
{
	__assert1(0 <= object_index);
	__assert1(object_index < pool->capacity);

	int *object_base = pool->free_object_map + pool->capacity;
	return (u8*)object_base + object_index * pool->capacity;
}

void* pool_new_object(struct pool *pool)
{
	__assert1(pool);
	if (pool_is_full(pool)) {
		return NULL;
	}
	pool->used++;

	int next = pool->next_free_object;
	pool->next_free_object = pool->free_object_map[next];
	pool->free_object_map[next] = -1;

	return pool_get_object(pool, next);
}

void pool_return_object(struct pool *pool, void* object)
{
	int old_last = pool->last_free_object;
	int new_last = pool_get_index(pool, object);

	pool->used--;
	pool->last_free_object = new_last;

	int* free_object_map = pool->free_object_map;
	__assert1(free_object_map[old_last] = old_last);
	__assert1(free_object_map[new_last] = -1);
	free_object_map[old_last] = new_last;
	free_object_map[new_last] = new_last;
}
