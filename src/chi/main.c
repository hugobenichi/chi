#include <chi.h>

struct editor {};

void editor_init(struct editor *editor, vec term_size)
{
}

void editor_render(struct editor *editor, struct framebuffer *framebuffer)
{
}

void resize(struct editor *editor, struct framebuffer *framebuffer)
{
	vec term_size = term_get_size();
	framebuffer_init(framebuffer, term_size);
	editor_init(editor, term_size);
}

void editor_process_input(struct editor *editor, struct input input)
{
}

static void print_input(struct input input)
{
	char *s = "UNKNOWN";
	char buffer[128] = {};
	if (is_printable_key(input.code)) {
		sprintf(buffer, "KEY:%c", input.code);
		s = buffer;
	}
	switch (input.code) {
	case CTRL_AT:			s = "CTRL_AT"; break;
	case CTRL_A:			s = "CTRL_A"; break;
	case CTRL_B: 			s = "CTRL_B"; break;
	case CTRL_C: 			s = "CTRL_C"; break;
	case CTRL_D: 			s = "CTRL_D"; break;
	case CTRL_E: 			s = "CTRL_E"; break;
	case CTRL_F: 			s = "CTRL_F"; break;
	case CTRL_G: 			s = "CTRL_G"; break;
	case CTRL_H: 			s = "CTRL_H"; break;
	case CTRL_I: 			s = "CTRL_I"; break;
	case CTRL_J: 			s = "CTRL_J"; break;
	case CTRL_K: 			s = "CTRL_K"; break;
	case CTRL_L: 			s = "CTRL_L"; break;
	case CTRL_M: 			s = "CTRL_M"; break;
	case CTRL_N: 			s = "CTRL_N"; break;
	case CTRL_O: 			s = "CTRL_O"; break;
	case CTRL_P: 			s = "CTRL_P"; break;
	case CTRL_Q: 			s = "CTRL_Q"; break;
	case CTRL_R: 			s = "CTRL_R"; break;
	case CTRL_S: 			s = "CTRL_S"; break;
	case CTRL_T: 			s = "CTRL_T"; break;
	case CTRL_U: 			s = "CTRL_U"; break;
	case CTRL_V: 			s = "CTRL_V"; break;
	case CTRL_W: 			s = "CTRL_W"; break;
	case CTRL_X: 			s = "CTRL_X"; break;
	case CTRL_Y: 			s = "CTRL_Y"; break;
	case CTRL_Z: 			s = "CTRL_Z"; break;
	case CTRL_LEFT_BRACKET:		s = "CTRL_LEFT_BRACKET"; break;
	case CTRL_BACKSLASH:		s = "CTRL_BACKSLASH"; break;
	case CTRL_RIGHT_BRACKET:	s = "CTRL_RIGHT_BRACKET"; break;
	case CTRL_CARET:		s = "CTRL_CARET"; break;
	case CTRL_UNDERSCORE:		s = "CTRL_UNDERSCORE"; break;
	case SPACE:			s = "SPACE"; break;
	case DEL:                       s = "DEL"; break;
	case INPUT_RESIZE_CODE:         s = "RESIZE"; break;
	case INPUT_KEY_ESCAPE_Z:        s = "ESC_Z"; break;
	case INPUT_KEY_ARROW_UP:        s = "UP"; break;
	case INPUT_KEY_ARROW_DOWN:      s = "DOWN"; break;
	case INPUT_KEY_ARROW_RIGHT:     s = "RIGHT"; break;
	case INPUT_KEY_ARROW_LEFT:      s = "LEFT"; break;
	case INPUT_MOUSE_LEFT:          s = "MOUSE L"; break;
	case INPUT_MOUSE_MIDDLE:        s = "MOUSE MID"; break;
	case INPUT_MOUSE_RIGHT:         s = "MOUSE R"; break;
	case INPUT_MOUSE_RELEASE:       s = "MOUSE RELEASE"; break;
	case INPUT_ERROR_CODE:          s = "ERROR"; break;
	default:			break;
	}
	printf("%s", s);
	switch (input.code) {
	case INPUT_MOUSE_LEFT:
	case INPUT_MOUSE_MIDDLE:
	case INPUT_MOUSE_RIGHT:
	case INPUT_MOUSE_RELEASE:
		printf(": %i,%i", input.mouse_click.y, input.mouse_click.x);
	default:
		break;
	}
	printf("\n\r");
}

struct err foo() { return error_because(EINVAL); }
//struct err foo() { return noerror(); }
struct err foo1() { struct err e = foo(); return_if_error(e); return noerror(); }
struct err foo2() { struct err e = foo1(); return_if_error(e); return noerror(); }
struct err foo3() { struct err e = foo2(); return_if_error(e); return noerror(); }
static void err_stack_trace_example()
{
	struct err err = foo3();
	if (err.is_error) {
		printf("error: %s\n", error_msg(err));
		struct err *e = &err;
		while (e) {
			// TODO: print cause
			printf("  %s:%s\n", e->loc, e->func);
			e = e->cause;
		}
		err_acknoledge(err);
	}
}
#include <assert.h>

int main(int argc, char **args) {
	log_init();
	config_init();
	term_init();

	struct framebuffer framebuffer = {};
	struct editor editor = {};
	resize(&editor, &framebuffer);

	for (;;) {
		struct input input = term_get_input();
		print_input(input);
		switch (input.code) {
		case INPUT_RESIZE_CODE:
			resize(&editor, &framebuffer);
			break;
		case CTRL_C:
			// TODO: confirmation for saving buffers with pending changes.
			return 0;
		default:
			break;
		}
		editor_render(&editor, &framebuffer);


		char buffer[256] = {};
		framebuffer_print(buffer, 256, &framebuffer);
		puts(buffer);
		framebuffer_draw_to_term(&framebuffer, v(0,0));
	}
}
