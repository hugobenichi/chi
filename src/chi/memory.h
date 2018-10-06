#ifndef __chi_memory__
#define __chi_memory__


#include <string.h>

#include <chi/base.h>


// Pair of pointer that delimits a memory segment.
struct slice {
	char *start;
	char *stop;
};

// Delimits a memory segment
struct memory {
	char *memory;
	size_t size;
};

// A memory buffer with an internal cursor for tracking appends.
struct buffer {
	char *memory;
  size_t cursor;
	size_t size;
};

#define slice_wrap_string(string) s(string, string + sizeof(string) - 1)


// TODO: write generic len, append, copy functions that works with different
// combos of slice, memory and buffer.

struct slice s(char *start, char *stop)
{
	return (struct slice) {
		.start = start,
		.stop = stop,
	};
}

static inline size_t slice_len(struct slice s)
{
	return s.stop - s.start;
}

static inline struct slice buffer_to_slice(struct buffer b)
{
	return s(b.memory, b.memory + b.size);
}

static struct slice slice_copy(struct slice dst, const char* src, size_t srclen)
{
	size_t dstlen = slice_len(dst);
	size_t len = min(dstlen, srclen);
	memcpy(dst.start, src, len);
	return s(dst.start + len, dst.stop);
}
#define slice_copy_string(dst, string) slice_copy(dst, string, sizeof(string) - 1)

static struct slice slice_copys(struct slice dst, const struct slice src)
{
  return slice_copy(dst, src.start, slice_len(src));
}

static inline size_t buffer_capacity(struct buffer dst) {
  return dst.size - dst.cursor;
}

static inline char* buffer_end(struct buffer dst) {
  return dst.memory + dst.cursor;
}


static void buffer_append(struct buffer *dst, const char* src, size_t srclen)
{
	size_t len = min(buffer_capacity(*dst), srclen);
	memcpy(dst->memory, src, len);
}


#endif
