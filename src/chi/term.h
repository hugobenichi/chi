#ifndef __chi_term__
#define __chi_term__

#include <chi/geometry.h>
#include <chi/memory.h>
#include <chi/input.h>

struct term_framebuffer {
};

void term_init();
void term_framebuffer_init(struct term_framebuffer* framebuffer, vec term_size);
void term_draw(struct term_framebuffer *framebuffer);
vec term_get_size();
struct input term_get_input();

#endif
