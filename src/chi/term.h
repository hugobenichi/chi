#ifndef __chi_term__
#define __chi_term__

#include <chi/geometry.h>
#include <chi/memory.h>
#include <chi/input.h>

void term_init();
vec term_get_size();
struct input term_get_input();

#define colors

struct framebuffer {
  vec window;                     // size of the display
  size_t buffer_len;              // size of the various buffers.
  c8 *text;                       // 2D buffer for storing text.
  int *fg_colors;                 // 2d buffer for storing foreground colors,
  int *bg_colors;                 // 2d buffer for storing foreground colors,
  struct buffer output_buffer;    // append buffer for storing all control sequences and output text to the terminal for one frame.
};

void framebuffer_init(struct framebuffer* framebuffer, vec term_size);
void framebuffer_draw_to_term(struct framebuffer *framebuffer);

#endif
