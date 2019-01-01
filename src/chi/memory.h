#ifndef __chi_memory__
#define __chi_memory__


#include <unistd.h>
#include <string.h>

#include <chi/base.h>


// TODO: write generic len, append, copy functions that works with different
// combos of slice, memory and buffer.

// Pair of pointer that delimits a memory segment.
struct slice {
	u8 *start;
	u8 *stop;
};
typedef struct slice slice;

// Delimits a memory segment
struct memory {
	char *memory;
	size_t size;
};
typedef struct memory memory;

// A memory buffer with an internal cursor for tracking appends.
struct buffer {
	char *memory;
  size_t cursor;
	size_t size;
};
typedef struct buffer buffer;

#define slice_wrap_string(string) s(string, string + sizeof(string) - 1)




// Slice operations

static inline struct slice s(char *start, char *stop)
{
	return (struct slice) {
		.start = start,
		.stop = stop,
	};
}

static inline struct slice s2(void* memory, size_t len)
{
	return (struct slice) {
		.start = memory,
		.stop = memory + len,
	};
}

static inline size_t slice_len(struct slice s)
{
	return s.stop - s.start;
}

static inline char* slice_to_string(struct slice s)
{
  size_t l = slice_len(s);
  char* string = malloc(l + 1);
  if (string) {
    memcpy(string, s.start, l);
    *(string + l) = 0;
  }
  return string;
}

static inline size_t slice_empty(struct slice s)
{
	return s.stop == s.start;
}

static inline int slice_write(struct slice s, int fd)
{
  return write(fd, s.start, slice_len(s));
}

static inline int slice_read(struct slice s, int fd)
{
  return read(fd, s.start, slice_len(s));
}

static inline size_t slice_copy(struct slice dst, const struct slice src)
{
	size_t len = min(slice_len(dst), slice_len(src));
	memcpy(dst.start, src.start, len);
	return len;
}

#define slice_around_value(v) s2(&v, sizeof(v))
#define slice_copy_value(dst, v) slice_copy(dst, slize_around_value(v))

#define slice_around_ref(p) s2(p, sizeof(*p))
#define slice_copy_ref(dst, p) slice_copy(dst, slize_around_ref(p))

// TODO: rework the interface: this just cut a slice at n and both drop/take should be the same thing.
static inline struct slice slice_drop(struct slice s, int n)
{
  n = min(n, slice_len(s));
  s.start = s.start - n;
  return s;
}

static inline struct slice slice_take(struct slice s, int n)
{
  n = min(n, slice_len(s));
  s.stop = s.stop - n;
  return s;
}

static inline struct slice slice_strip(struct slice s, u8 c)
{
  while (slice_len(s) && *(s.stop - 1) == c) {
    s.stop--;
  }
  return s;
}

static inline struct slice slice_split(struct slice *s, int c)
{
  struct slice left = *s;
  u8* pivot = (u8*) memchr(left.start, c, slice_len(left));
  if (!pivot) {
    pivot = s->stop;
  }

  left.stop = pivot;
  s->start = pivot;

  return left;
}

static inline struct slice slice_take_line(struct slice *s)
{
  struct slice line = slice_split(s, '\n');
  if (*s->start == '\n') {
    s->start++;
  }
  return line;
}

static inline struct slice slice_while(struct slice *s, int fn(u8))
{
  u8* u = s->start;
  while (u < s->stop && fn(*u)) {
    u++;
  }
  struct slice left = *s;
  left.stop = u;
  s->start = u;
  return left;
}


// Buffer operations

static inline struct slice buffer_to_slice(struct buffer b)
{
	return s(b.memory, b.memory + b.size);
}

static struct slice slice_copy_bytes(struct slice dst, const char* src, size_t srclen)
{
	size_t dstlen = slice_len(dst);
	size_t len = min(dstlen, srclen);
	memcpy(dst.start, src, len);
	return s(dst.start + len, dst.stop);
}
#define slice_copy_string(dst, string) slice_copy_bytes(dst, string, sizeof(string) - 1)

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


//#define mylen(a) _Generic((a), struct slice: ((size_t)((a).stop	- (a).start)))

#define mylen(a) _Generic((a),          \
	struct slice:   ((size_t)((a).stop	- (a).start)),  \
	struct memory:	(a).size,   \
	struct buffer:	(a).cursor)


#endif
