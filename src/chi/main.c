#include <chi/base.h>
#include <chi/config.h>
#include <chi/error.h>
#include <chi/log.h>
#include <chi/term.h>

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
		switch (input.type) {
			case INPUT_RESIZE:
				resize(&editor, &framebuffer);
				break;
			case INPUT_KEY:
			case INPUT_MOUSE:
				editor_process_input(&editor, input);
				break;
			case INPUT_NONE:
			case INPUT_UNKNOWN:
				break;
		}
		editor_render(&editor, &framebuffer);
		term_draw(&framebuffer);
	}
}
