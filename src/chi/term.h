#ifndef __chi_term__
#define __chi_term__

#include <chi/geometry.h>
#include <chi/memory.h>
#include <chi/input.h> // TODO: fuse this other file back in there

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


#endif
