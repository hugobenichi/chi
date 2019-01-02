#include <chi/base.h>
#include <chi/config.h>
#include <chi/error.h>
#include <chi/log.h>
#include <chi/term.h>
#include <chi/input.h>

struct editor {};

void editor_init(struct editor *editor, vec term_size)
{
}

void editor_render(struct editor *editor, struct term_framebuffer *framebuffer)
{
}

void resize(struct editor *editor, struct term_framebuffer *framebuffer)
{
	vec term_size = term_get_size();
	term_framebuffer_init(framebuffer, term_size);
	editor_init(editor, term_size);
}

void editor_process_input(struct editor *editor, struct input input)
{
}

int main(int argc, char **args) {
	log_init();
	config_init();
	term_init();

	struct term_framebuffer framebuffer = {};
	struct editor editor = {};
	resize(&editor, &framebuffer);

	for (;;) {
		struct input input = term_get_input();
		switch (input.code) {
		case INPUT_RESIZE_CODE:
			resize(&editor, &framebuffer);
			break;
		case CTRL_C:
			// TODO: confirmation for saving buffers with pending changes.
			return 0;
		}
		editor_render(&editor, &framebuffer);
		term_draw(&framebuffer);
	}
}
