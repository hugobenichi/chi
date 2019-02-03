#ifndef __chi__
#define __chi__

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <execinfo.h>
#include <sys/stat.h>


/// module BASE ///

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef char      c8;


#define Kilo(x) (1024L * (x))
#define Mega(x) (1024L * Kilo(x))
#define Giga(x) (1024L * Mega(x))

#define __stringize1(x) #x
#define __stringize2(x) __stringize1(x)
#define __LOC__ __FILE__ ":" __stringize2(__LINE__)

// Careful: the result expr gets evaluated twice
#define max(x,y) ((x) > (y) ? (x) : (y))
#define min(x,y) ((x) < (y) ? (x) : (y))

// Find statically the size of an array literal.
#define _arraylen(ary)    (sizeof(ary)/sizeof(ary[0]))

// Find statically the size of a __VA_ARGS__ list passed to a macro.
#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))


// Print formatted error msg and exit.
static inline void fatal(const char * format, ...)
{
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
        exit(1);
}

// More asserts
// TODO: replace strerror with a mapping of errno values to errno symbols like ENOPERM.
#define assertf(condition, format, ...) if (!(condition)) fatal(__LOC__ " :%s: " format, __func__, ##__VA_ARGS__)
#define assert_success(condition) assertf(-1 < condition, "assert failed: " __stringize2(condition) " failed with: %s", strerror(errno))
#define assertf_success(condition, format, ...) if (condition) fatal(__LOC__ " %s: %s: " format, __func__, strerror(errno), ##__VA_ARGS__)


enum err_code {
  CHI_ERR_GENERIC = 1000,
};

struct err {
  union {
    int errno_val;
    int is_error;
  };
  const char *loc;
  const char *func;
  struct err *cause;
};

static inline struct err err_new_proto(struct err err)
{
  if (err.cause) {
    struct err *cause_copy = malloc(sizeof(struct err));
    memcpy(cause_copy, err.cause, sizeof(struct err));
    err.errno_val = err.cause->errno_val;
    err.cause = cause_copy;
  }
  return err;
}

static inline void err_acknoledge(struct err err)
{
  struct err *e = err.cause;
  while (e) {
    struct err *f = e;
    e = e->cause;
    free(f);
  }
}

static inline const char* error_msg(struct err err)
{
    switch (err.errno_val) {
    case 0:                           return "Success";
    // TODO: cover all normal errno values
    case EINVAL:                      return "EINVAL";
    case CHI_ERR_GENERIC:             return "Chi generic error";
    default:                          return "Unknown error";
    }
}

#define error(...) err_new_proto((struct err){ .errno_val=CHI_ERR_GENERIC, .loc=__LOC__, .func=__func__, ##__VA_ARGS__})
#define error_because(errno_v) err_new_proto((struct err){ .errno_val=errno_v, .loc=__LOC__, .func=__func__})
#define error_wrap(err) error(.cause=&err)
#define return_if_error(err) if (err.is_error) return error_wrap(err)
#define noerror() error_because(0)


/// module GEOMETRY ///

struct vec {
	int32_t x;
	int32_t y;
};
typedef struct vec vec;

struct rec {
	union {
		struct {
			vec min;
			vec max;
		};
		struct {
			int32_t x0;
			int32_t y0;
			int32_t x1;
			int32_t y1;
		};
	};
};
typedef struct rec rec;

static inline vec v(int32_t x ,int32_t y)
{
	return (vec){
		.x = x,
		.y = y,
	};
}

static inline rec r(vec min, vec max)
{
	return (rec){
		.min = min,
		.max = max,
	};
}

static inline vec sub_vec_vec(vec v0, vec v1)
{
	return v(v0.x - v1.x, v0.y - v1.y);
}

static inline vec rec_diag(rec r)
{
	return sub_vec_vec(r.max, r.min);
}

static inline int32_t rec_w(rec r)
{
	return r.x1 - r.x0;
}

static inline int32_t rec_h(rec r)
{
	return r.y1 - r.y0;
}

static inline vec add_vec_vec(vec v0, vec v1)
{
	return v(v0.x + v1.x, v0.y + v1.y);
}

static inline rec add_vec_rec(vec v0, rec r1)
{
	return r(add_vec_vec(v0, r1.min), add_vec_vec(v0, r1.max));
}

static inline rec add_rec_vec(rec r0, vec v1)
{
	return add_vec_rec(v1, r0);
}

static inline rec add_rec_rec(rec r0, rec r1)
{
	return r(add_vec_vec(r0.min, r1.min), add_vec_vec(r0.max, r1.max));
}

static inline rec sub_rec_vec(rec r0, vec v1)
{
	return r(sub_vec_vec(r0.min, v1), sub_vec_vec(r0.max, v1));
}

// Generic adder for vec and rec.
#define add(a,b) _Generic((a),          \
	rec:	add_rec_other(b),       \
	vec:	add_vec_other(b))((a),(b))

#define add_rec_other(b) _Generic((b),  \
	rec:	add_rec_rec,            \
	vec:	add_rec_vec)

#define add_vec_other(b) _Generic((b),  \
	rec:	add_vec_rec,            \
	vec:	add_vec_vec)

#define sub(a,b) _Generic((a),          \
	rec:	sub_rec_vec,            \
	vec:	sub_vec_vec)((a),(b))


static char* vec_print(char *dst, size_t len, vec v)
{
	int n = snprintf(dst, len, "{.x=%d, .y=%d}", v.x, v.y);
	return dst + n; //_min(n, len);
}

static char* rec_print(char *dst, size_t len, rec r)
{
	int32_t w = rec_w(r);
	int32_t h = rec_h(r);
	int n = snprintf(dst, len, "{.x0=%d, .y0=%d, .x1=%d, .y1=%d, w=%d, .h=%d}", r.x0, r.y0, r.x1, r.y1, w, h);
	return dst + n; //_min(n, len);
}


/// module MEMORY ///

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

static inline struct slice s(void *start, void *stop)
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
static inline struct slice slice_drop(struct slice s, size_t n)
{
  n = min(n, slice_len(s));
  s.start = s.start - n;
  return s;
}

static inline struct slice slice_take(struct slice s, size_t n)
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

static void buffer_ensure_size(struct buffer *buffer, size_t size)
{
  if (size <= buffer->size) {
    return;
  }
  buffer->memory = realloc(buffer->memory, size);
}

static void buffer_ensure_capacity(struct buffer *buffer, size_t additional_capacity)
{
  buffer_ensure_size(buffer, buffer->cursor + additional_capacity);
}

static void buffer_append(struct buffer *dst, const char* src, size_t srclen)
{
  buffer_ensure_capacity(dst, srclen);
	memcpy(buffer_end(*dst), src, srclen);
}

#define buffer_append_cstring(dst, string)  buffer_append(dst, string, strlen(string))

#define buffer_appendf(dst, format, ...) buffer_appendf_proto(dst, format, NUMARGS(__VA_ARGS__), __VA_ARGS__)
static void buffer_appendf_proto(struct buffer *dst, const char* format, int numargs, ...)
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



//#define mylen(a) _Generic((a), struct slice: ((size_t)((a).stop	- (a).start)))

#define mylen(a) _Generic((a),          \
	struct slice:   ((size_t)((a).stop	- (a).start)),  \
	struct memory:	(a).size,   \
	struct buffer:	(a).cursor)


/// module POOL ///

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


/// module IO ///

// DOCME
struct mapped_file {
	char name[256];
	struct stat file_stat;
	struct slice data;
};

// DOCME
int mapped_file_load(struct mapped_file *f, const char *path);


/// module ERROR ///

static int TRACE_BUFFER_MAX = 256;

static inline void backtrace_print()
{
  void* trace_buffer[TRACE_BUFFER_MAX];
  int depth = backtrace(trace_buffer, TRACE_BUFFER_MAX);

  char **trace_symbols = backtrace_symbols(trace_buffer, depth);
  for (int i = 0; i < depth; i++) {
    puts(trace_symbols[i]);
  }
  free(trace_symbols);
}

static inline void read_binary(const char *path)
{
	struct mapped_file binary = {};
	mapped_file_load(&binary, path);

  // TODO: parse ELF symbols
}


/// module CONFIG ///

struct config {
  int debug_noterm;
  int debug_config;
};

struct config CONFIG;

void config_init();


/// module LOG ///

// The fd value where logs are emitted. Goes to /dev/null by default.
const int LOG_FILENO;

void log_init();
int log_formatted_message(const char *format, ...);


#define logm(format, ...)  log_formatted_message(__LOC__ " %s [ " format, __func__, ##__VA_ARGS__)
#define logs(s)  logm("%s", s)


// module INPUT

enum input_type {
  INPUT_RESIZE,
  INPUT_KEY,
  INPUT_MOUSE,
  INPUT_ERROR,
  INPUT_UNKNOWN,
};

enum input_code {
  // all ascii control codes
  CTRL_AT,
  CTRL_A,
  CTRL_B,
  CTRL_C,
  CTRL_D,
  CTRL_E,
  CTRL_F,
  CTRL_G,
  CTRL_H,
  CTRL_I,
  CTRL_J,
  CTRL_K,
  CTRL_L,
  CTRL_M,
  CTRL_N,
  CTRL_O,
  CTRL_P,
  CTRL_Q,
  CTRL_R,
  CTRL_S,
  CTRL_T,
  CTRL_U,
  CTRL_V,
  CTRL_W,
  CTRL_X,
  CTRL_Y,
  CTRL_Z,
  CTRL_LEFT_BRACKET,
  CTRL_BACKSLASH,
  CTRL_RIGHT_BRACKET,
  CTRL_CARET,
  CTRL_UNDERSCORE,

  // first printable ascii code
  SPACE,
  // all other printable ascii codes implicitly included here.

  // last ascii code.
  DEL                         = '\x7f',

  // additional special codes representing other input sequences.
  INPUT_RESIZE_CODE           = 1000,
  INPUT_KEY_ESCAPE_Z          = 1001,
  INPUT_KEY_ARROW_UP          = 1002,
  INPUT_KEY_ARROW_DOWN        = 1003,
  INPUT_KEY_ARROW_RIGHT       = 1004,
  INPUT_KEY_ARROW_LEFT        = 1005,
  INPUT_MOUSE_LEFT            = 1006,
  INPUT_MOUSE_MIDDLE          = 1007,
  INPUT_MOUSE_RIGHT           = 1008,
  INPUT_MOUSE_RELEASE         = 1009,
  INPUT_ERROR_CODE            = 1010,
};

// Other "famous" keys mapping to control codes
static const int ESC                   = CTRL_LEFT_BRACKET;
static const int BACKSPACE             = CTRL_H;
static const int TAB                   = CTRL_I;
static const int LINE_FEED             = CTRL_J;
static const int VTAB                  = CTRL_K;
static const int NEW_PAGE              = CTRL_L;
static const int ENTER                 = CTRL_M;

struct input {
  enum input_code code;
  union {
    vec mouse_click;
    int errno_value;
  };
};

static inline enum input_type input_type_of(struct input input)
{
  if (input.code <= '\x7f') {
      return INPUT_KEY;
  }
  switch (input.code) {
  case INPUT_RESIZE_CODE:
      return INPUT_RESIZE;
  case INPUT_KEY_ESCAPE_Z:
  case INPUT_KEY_ARROW_UP:
  case INPUT_KEY_ARROW_DOWN:
  case INPUT_KEY_ARROW_RIGHT:
  case INPUT_KEY_ARROW_LEFT:
      return INPUT_KEY;
  case INPUT_MOUSE_LEFT:
  case INPUT_MOUSE_MIDDLE:
  case INPUT_MOUSE_RIGHT:
  case INPUT_MOUSE_RELEASE:
      return INPUT_MOUSE;
  case INPUT_ERROR_CODE:
      return INPUT_ERROR;
  default:
      return INPUT_UNKNOWN;
  }
}

static inline int is_printable_key(int code)
{
  return ' ' <= code && code <= '~';
}


/// module TERM ///

void term_init();
vec term_get_size();
struct input term_get_input();


struct framebuffer {
  vec window;                     // size of the display
  size_t buffer_len;              // size of the various buffers.
  c8 *text;                       // 2D buffer for storing text.
  int *fg_colors;                 // 2d buffer for storing foreground colors,
  int *bg_colors;                 // 2d buffer for storing foreground colors,
  struct buffer output_buffer;    // append buffer for storing all control sequences and output text to the terminal for one frame.
};

void framebuffer_init(struct framebuffer *framebuffer, vec term_size);
void framebuffer_draw_to_term(struct framebuffer *framebuffer, vec cursor);
void framebuffer_clear(struct framebuffer *framebuffer, rec rec);
void framebuffer_put_color_fg(struct framebuffer *framebuffer, int fg, rec rec);
void framebuffer_put_color_bg(struct framebuffer *framebuffer, int bg, rec rec);

// debugging functions
void framebuffer_print(char* buffer, size_t size, struct framebuffer* framebuffer);

// A framebuffer iterator allows to naviguate a subrectangle of a drawing framebuffer line by line, up or down.
// All framebuffer iterator functions mutates the iterator passed by pointer.
// However, it is safe to save a copy of an iterator and used it later.
// If a framebuffer is resized, all existing iterators are invalidated.
// Initially the iterator is set one step before the first line, it is necessary to 

struct framebuffer_iter {
  c8 *text;
  int *fg;
  int *bg;
  size_t stride;
  size_t line_length;
  int line_max;
  int line_current;
};

struct framebuffer_iter framebuffer_iter_make(struct framebuffer *framebuffer, rec rec);
// Adds a constant horizontal offset, dropping framebuffer slots on the left.
void framebuffer_iter_offset(struct framebuffer_iter *iter, size_t offset);
void framebuffer_iter_reset_first(struct framebuffer_iter *iter);
void framebuffer_iter_reset_last(struct framebuffer_iter *iter);
// Both these function returns true if the iterator was moved  can still be moved further in the same direction.
int framebuffer_iter_next(struct framebuffer_iter *iter);
int framebuffer_iter_prev(struct framebuffer_iter *iter);
// These three functions return the number of slots into which bytes/colors were pushed.
size_t framebuffer_push_text(struct framebuffer_iter *iter, c8 *text, size_t size);
size_t framebuffer_push_fg(struct framebuffer_iter *iter, int fg, size_t size);
size_t framebuffer_push_bg(struct framebuffer_iter *iter, int bg, size_t size);


/// module TEXTBUFFER ///

void textbuffer_init();

// A single line of text, made of a linked list of struct slices.
// Lines are linked together in a doubly-linked lists for simple navigation and insertions.
struct line {
  struct line *prev;
  struct line *next;
  struct textpiece {
    struct textpiece *next;
    struct slice slice;
  } fragment;
  size_t bytelen;
};

// A cursor pointing into a single location into a single line of text.
// It retains a link to its owning textbuffer.
struct cursor {
  struct textbuffer *textbuffer;
  struct line *line;
  // TODO: change to lineno and x_offset ?
  vec position;
};


// this is just an opaque handle
struct textbuffer {
};


// TODO: define all ops
enum textbuffer_op {
  TEXTBUFFER_OPEN,
  TEXTBUFFER_TOUCH,
  TEXTBUFFER_SAVE,
  TEXTBUFFER_CLOSE,

  TEXTBUFFER_MAX,
};

struct textbuffer_command {
  int op;
  int arg_size;
  void* args;
};

struct err textbuffer_operation(struct textbuffer textbuffer, struct textbuffer_command *command);


#endif
