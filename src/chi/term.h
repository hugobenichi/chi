#ifndef __chi_term__
#define __chi_term__

#include <chi/geometry.h>
#include <chi/memory.h>

vec term_get_size();

void term_init();

struct term_frame {
	// TODO
};

void term_render(struct buffer buffer, struct term_frame *frame);

#endif
