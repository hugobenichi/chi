// mem.c implements operations for memory related objects:
//	slice
//	buffer
//
#include <chi.h>

struct slice s(void *start, void *stop)
{
	return (struct slice) {
		.start = start,
		.stop = stop,
	};
}

struct slice s2(void* memory, size_t len)
{
	return (struct slice) {
		.start = memory,
		.stop = memory + len,
	};
}

size_t slice_len(struct slice s)
{
	return s.stop - s.start;
}

char* slice_to_string(struct slice s)
{
  size_t l = slice_len(s);
  char* string = malloc(l + 1);
  if (string) {
    memcpy(string, s.start, l);
    *(string + l) = 0;
  }
  return string;
}

size_t slice_empty(struct slice s)
{
	return s.stop == s.start;
}

int slice_write(struct slice s, int fd)
{
  return write(fd, s.start, slice_len(s));
}

int slice_read(struct slice s, int fd)
{
  return read(fd, s.start, slice_len(s));
}

size_t slice_copy(struct slice dst, const struct slice src)
{
	size_t len = min(slice_len(dst), slice_len(src));
	memcpy(dst.start, src.start, len);
	return len;
}

struct slice slice_drop(struct slice s, size_t n)
{
  n = min(n, slice_len(s));
  s.start = s.start - n;
  return s;
}

struct slice slice_take(struct slice s, size_t n)
{
  n = min(n, slice_len(s));
  s.stop = s.stop - n;
  return s;
}

struct slice slice_strip(struct slice s, u8 c)
{
  while (slice_len(s) && *(s.stop - 1) == c) {
    s.stop--;
  }
  return s;
}

struct slice slice_split(struct slice *s, int c)
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

struct slice slice_take_line(struct slice *s)
{
  struct slice line = slice_split(s, '\n');
  if (*s->start == '\n') {
    s->start++;
  }
  return line;
}

struct slice slice_while(struct slice *s, int fn(u8))
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


struct slice buffer_to_slice(struct buffer b)
{
	return s(b.memory, b.memory + b.size);
}

struct slice slice_copy_bytes(struct slice dst, const char* src, size_t srclen)
{
	size_t dstlen = slice_len(dst);
	size_t len = min(dstlen, srclen);
	memcpy(dst.start, src, len);
	return s(dst.start + len, dst.stop);
}

size_t buffer_capacity(struct buffer dst)
{
  return dst.size - dst.cursor;
}

char* buffer_end(struct buffer dst)
{
  return dst.memory + dst.cursor;
}

void buffer_ensure_size(struct buffer *buffer, size_t size)
{
  if (size <= buffer->size) {
    return;
  }
  buffer->memory = realloc(buffer->memory, size);
}

void buffer_ensure_capacity(struct buffer *buffer, size_t additional_capacity)
{
  buffer_ensure_size(buffer, buffer->cursor + additional_capacity);
}

void buffer_append(struct buffer *dst, const char* src, size_t srclen)
{
  buffer_ensure_capacity(dst, srclen);
	memcpy(buffer_end(*dst), src, srclen);
}


void buffer_appendf_proto(struct buffer *dst, const char* format, int numargs, ...)
{
  va_list args;
  va_start(args, numargs);

  for (;;) {
    size_t capacity = buffer_capacity(*dst);
    size_t n = vsnprintf(buffer_end(*dst), capacity, format, args);
    if (capacity < n) {
        buffer_ensure_capacity(dst, n);
        continue;
    }
    dst->cursor += n;
    break;
  }

  va_end(args);
}
