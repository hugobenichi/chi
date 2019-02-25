// mem.c implements operations for memory related objects:
//	slice
//	buffer
//
#include <chi.h>

slice s(void *start, void *stop)
{
	return (slice) {
		.start = start,
		.stop = stop,
	};
}

size_t slice_len(slice s)
{
	return s.stop - s.start;
}

char* slice_to_string(slice s)
{
	size_t l = slice_len(s);
	char *string = malloc(l + 1);
	if (string) {
		memcpy(string, s.start, l);
		*(string + l) = 0;
	}
	return string;
}

int slice_empty(slice s)
{
	return s.stop <= s.start;
}

int slice_write(slice s, int fd)
{
	return write(fd, s.start, slice_len(s));
}

int slice_read(slice s, int fd)
{
	return read(fd, s.start, slice_len(s));
}

size_t slice_copy(slice dst, const slice src)
{
	size_t len = min(slice_len(dst), slice_len(src));
	memcpy(dst.start, src.start, len);
	return len;
}

slice slice_drop(slice s, size_t n)
{
	n = min(n, slice_len(s));
	s.start = s.start - n;
	return s;
}

slice slice_take(slice s, size_t n)
{
	n = min(n, slice_len(s));
	s.stop = s.stop - n;
	return s;
}

slice slice_strip(slice s, char c)
{
	while (slice_len(s) && *(s.stop - 1) == c) {
		s.stop--;
	}
	return s;
}

// Split the given slice into two at the first occurence of 'c'.
// The left side is returned as a value, the given slice is mutated to match the right side.
// The pivot is included in the right.
// However, if 'c' is not found in the slice, everything is taken and the right side becomes empty.
slice slice_split(slice *s, int c)
{
	slice left = *s;
	char *pivot = (char *) memchr(left.start, c, slice_len(left));
	if (!pivot) {
		pivot = s->stop;
	}

	left.stop = pivot;
	s->start = pivot;

	return left;
}

slice slice_take_line(slice *s)
{
	slice line = slice_split(s, '\n');
	if (*s->start == '\n') {
		s->start++;
	}
	return line;
}

slice slice_while(slice *s, int fn(char))
{
	char *u = s->start;
	while (u < s->stop && fn(*u)) {
		u++;
	}
	slice left = *s;
	left.stop = u;
	s->start = u;
	return left;
}

buffer b(void* memory, size_t len)
{
	return (buffer) {
		.memory = memory,
		.cursor = 0,
		.size = len,
	};
}

slice slice_copy_bytes(slice dst, const char *src, size_t srclen)
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

void buffer_append(struct buffer *dst, const char *src, size_t srclen)
{
	buffer_ensure_capacity(dst, srclen);
	memcpy(buffer_end(*dst), src, srclen);
}


void buffer_appendf_proto(struct buffer *dst, const char *format, int numargs, ...)
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

ssize_t buffer_read(buffer *dst, int fd,  size_t size)
{
	ssize_t r = read(fd, dst->memory + dst->cursor, min(size, buffer_capacity(*dst)));
	if (r > 0) {
		dst->cursor += r;
	}
	return r;
}

slice buffer_to_slice(struct buffer b)
{
	return s(b.memory, b.memory + b.cursor);
}

slice buffer_to_slice_full(struct buffer b)
{
	return s(b.memory, b.memory + b.size);
}

buffer buffer_from_slice(slice s)
{
	return b(s.start, slice_len(s));
}

size_t copy_slice_slice(slice dst, slice src)
{
	return slice_copy(dst, src);
}

size_t copy_slice_buffer(slice dst, buffer src)
{
	return slice_copy(dst, buffer_to_slice(src));
}

size_t copy_slice_buffer_full(slice dst, buffer src)
{
	return slice_copy(dst, buffer_to_slice_full(src));
}
